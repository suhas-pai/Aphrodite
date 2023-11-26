/*
 * kernel/src/mm/pgmap.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/align.h"

#include "mm/page_alloc.h"
#include "mm/walker.h"

#include "pgmap.h"

enum map_result {
    MAP_DONE,
    MAP_CONTINUE,
    MAP_RESTART
};

struct current_split_info {
    struct range phys_range;
    uint64_t virt_addr;

    bool is_active : 1;
};

#define CURRENT_SPLIT_INFO_INIT() \
    ((struct current_split_info){ \
        .phys_range = RANGE_EMPTY(), \
        .virt_addr = 0, \
        .is_active = false \
    })

static void
split_large_page(struct pt_walker *const walker,
                 struct pageop *const pageop,
                 struct current_split_info *const curr_split,
                 const pgt_level_t level,
                 const struct pgmap_options *const options)
{
    assert(!curr_split->is_active);

    pte_t *const table = walker->tables[walker->level - 1];
    pte_t *const pte = &table[walker->indices[walker->level - 1]];

    const pte_t entry = pte_read(pte);

    pte_write(pte, /*value=*/0);
    ptwalker_deref_from_level(walker, walker->level, pageop);

    curr_split->virt_addr = ptwalker_get_virt_addr(walker);
    curr_split->phys_range =
        RANGE_INIT(pte_to_phys(entry), PAGE_SIZE_AT_LEVEL(walker->level));

    if (!options->is_in_early) {
        pageop_flush_pte_in_current_range(pageop,
                                          entry,
                                          walker->level,
                                          options->free_pages);
    }

    for (pgt_level_t i = 1; i <= level; i++) {
        walker->tables[i - 1] = NULL;
        walker->indices[i - 1] = 0;
    }

    curr_split->is_active = true;
}

static bool
pgmap_with_ptwalker(struct pt_walker *const walker,
                    struct current_split_info *const curr_split,
                    struct pageop *const pageop,
                    const struct range phys_range,
                    const uint64_t virt_addr,
                    const struct pgmap_options *options);

__optimize(3) static void
finish_split_info(struct pt_walker *const walker,
                  struct current_split_info *const curr_split,
                  struct pageop *const pageop,
                  const uint64_t virt,
                  const struct pgmap_options *const options)
{
    if (!curr_split->is_active) {
        return;
    }

    const uint64_t virt_end =
        check_add_assert(curr_split->virt_addr, curr_split->phys_range.size);

    const uint64_t walker_virt_addr = ptwalker_get_virt_addr(walker);
    if (walker_virt_addr >= virt_end) {
        if (curr_split->phys_range.front != 0) {
            if (options->free_pages) {
                deref_page(phys_to_page(curr_split->phys_range.front), pageop);
            }
        }

        return;
    }

    struct page *const begin = phys_to_page(curr_split->phys_range.front);
    const struct page *const end =
        begin + PAGE_COUNT(curr_split->phys_range.size);

    struct page *page = begin + PAGE_COUNT(virt_end - walker_virt_addr);
    for (; page != end; page++) {
        ref_up(&page->largetail.refcount);
    }

    const uint64_t offset = virt_end - walker_virt_addr;
    const struct range phys_range =
        range_from_index(curr_split->phys_range, offset);

    struct current_split_info new_curr_split = CURRENT_SPLIT_INFO_INIT();
    pgmap_with_ptwalker(walker,
                        &new_curr_split,
                        pageop,
                        phys_range,
                        virt,
                        options);

    curr_split->is_active = false;
}

enum override_result {
    OVERRIDE_OK,
    OVERRIDE_DONE
};

static enum override_result
override_pte(struct pt_walker *const walker,
             struct current_split_info *const curr_split,
             struct pageop *const pageop,
             const uint64_t phys_begin,
             const uint64_t virt_begin,
             uint64_t *const offset_in,
             const uint64_t size,
             const pgt_level_t level,
             const pte_t new_pte_value,
             const struct pgmap_options *const options,
             const bool is_alloc_mapping)
{
    (void)virt_begin;
    if (level > walker->level) {
        pte_t *const pte =
            &walker->tables[level - 1][walker->indices[level - 1]];

        const pte_t entry = pte_read(pte);

        pte_write(pte, new_pte_value);
        pageop_flush_pte_in_current_range(pageop,
                                          entry,
                                          level,
                                          options->free_pages);

        return OVERRIDE_OK;
    }

    pte_t entry = 0;
    if (level < walker->level) {
        if (!ptwalker_points_to_largepage(walker)) {
            const enum pt_walker_result ptwalker_result =
                ptwalker_fill_in_to(walker,
                                    level,
                                    /*should_ref=*/!options->is_in_early,
                                    options->alloc_pgtable_cb_info,
                                    options->free_pgtable_cb_info);

            if (__builtin_expect(ptwalker_result != E_PT_WALKER_OK, 0)) {
                panic("mm: failed to pgmap, result=%d\n", ptwalker_result);
            }

            pte_t *const pte =
                &walker->tables[level - 1][walker->indices[level - 1]];

            pte_write(pte, new_pte_value);
            return OVERRIDE_OK;
        }

        const pgt_index_t index = walker->indices[walker->level - 1];
        entry = pte_read(&walker->tables[walker->level - 1][index]);
    } else {
        pte_t *const pte =
            &walker->tables[level - 1][walker->indices[level - 1]];

        entry = pte_read(pte);
        if (!pte_is_present(entry)) {
            pte_write(pte, new_pte_value);
            return OVERRIDE_OK;
        }
    }

    // Avoid overwriting if we have a page of the same size, or greater size,
    // that is mapped with the same flags.

    uint64_t phys_addr = phys_begin + *offset_in;
    if (!is_alloc_mapping &&
        pte_to_phys(entry) == phys_addr &&
        pte_flags_equal(entry, walker->level, options->pte_flags))
    {
        *offset_in += PAGE_SIZE_AT_LEVEL(walker->level);
        if (*offset_in >= size) {
            return OVERRIDE_DONE;
        }

        const struct pt_walker_iterate_options iterate_options = {
            .alloc_pgtable_cb_info = NULL,
            .free_pgtable_cb_info = NULL,

            .alloc_parents = false,
            .alloc_level = false,
            .should_ref = !options->is_in_early,
        };

        const enum pt_walker_result result =
            ptwalker_next_with_options(walker, walker->level, &iterate_options);

        if (__builtin_expect(result != E_PT_WALKER_OK, 0)) {
            panic("mm: failed to pgmap, result=%d", result);
        }

        *offset_in = phys_addr - phys_begin;
        return OVERRIDE_OK;
    }

    if (level < walker->level) {
        split_large_page(walker, pageop, curr_split, level, options);
    } else {
        //pageop_flush_address(virt_begin + *offset_in);
    }

    pte_t *const pte = &walker->tables[level - 1][walker->indices[level - 1]];
    pte_write(pte, new_pte_value);

    return OVERRIDE_OK;
}

__optimize(3) enum map_result
map_normal(struct pt_walker *const walker,
           struct current_split_info *const curr_split,
           struct pageop *const pageop,
           const uint64_t phys_begin,
           const uint64_t virt_begin,
           uint64_t *const offset_in,
           const uint64_t size,
           const struct pgmap_options *const options)
{
    uint64_t offset = *offset_in;
    if (__builtin_expect(offset >= size, 0)) {
        return MAP_DONE;
    }

    const bool should_ref = !options->is_in_early;
    const uint64_t pte_flags = options->pte_flags;

    enum pt_walker_result ptwalker_result = E_PT_WALKER_OK;
    struct pt_walker_iterate_options iterate_options = {
        .alloc_pgtable_cb_info = options->alloc_pgtable_cb_info,
        .free_pgtable_cb_info = options->free_pgtable_cb_info,

        .alloc_parents = false,
        .alloc_level = false,
        .should_ref = should_ref,
    };

    if (options->is_overwrite) {
        do {
            ptwalker_result =
                ptwalker_fill_in_to(walker,
                                    /*level=*/1,
                                    should_ref,
                                    options->alloc_pgtable_cb_info,
                                    options->free_pgtable_cb_info);

            if (__builtin_expect(ptwalker_result != E_PT_WALKER_OK, 0)) {
            panic:
                panic("mm: failed to pgmap, result=%d\n", ptwalker_result);
            }

            do {
                const pte_t new_pte_value =
                    phys_create_pte(phys_begin + offset) |
                    PTE_LEAF_FLAGS |
                    pte_flags;

                const enum override_result override_result =
                    override_pte(walker,
                                 curr_split,
                                 pageop,
                                 phys_begin,
                                 virt_begin,
                                 &offset,
                                 size,
                                 /*level=*/1,
                                 new_pte_value,
                                 options,
                                 /*is_alloc_mapping=*/false);

                switch (override_result) {
                    case OVERRIDE_OK:
                        break;
                    case OVERRIDE_DONE:
                        return MAP_DONE;
                }

                const bool should_fill_in =
                    walker->indices[1] != PGT_PTE_COUNT(2) - 1;

                iterate_options.alloc_parents = should_fill_in;
                iterate_options.alloc_level = should_fill_in;

                ptwalker_result =
                    ptwalker_next_with_options(walker,
                                               /*level=*/1,
                                               &iterate_options);

                if (__builtin_expect(ptwalker_result != E_PT_WALKER_OK, 0)) {
                    goto panic;
                }

                offset += PAGE_SIZE;
                if (offset == size) {
                    *offset_in = offset;
                    return MAP_DONE;
                }

                if (walker->indices[0] == 0) {
                    // Exit if the level above is at index 0, which may mean
                    // that a large page can be placed at the higher level.

                    if (!should_fill_in) {
                        *offset_in = offset;
                        return MAP_RESTART;
                    }

                    break;
                }
            } while (true);
        } while (true);
    } else {
        uint64_t phys_addr = phys_begin + offset;
        const uint64_t phys_end = phys_begin + size;

        ptwalker_result =
            ptwalker_fill_in_to(walker,
                                /*level=*/1,
                                should_ref,
                                options->alloc_pgtable_cb_info,
                                options->free_pgtable_cb_info);

        if (__builtin_expect(ptwalker_result != E_PT_WALKER_OK, 0)) {
            panic("mm: failed to pgmap, result=%d\n", ptwalker_result);
        }

        do {
            pte_t *const table = walker->tables[0];
            pte_t *pte = &table[walker->indices[0]];
            const pte_t *const end = &table[PGT_PTE_COUNT(1)];

            do {
                const pte_t new_pte_value =
                    phys_create_pte(phys_addr) | PTE_LEAF_FLAGS | pte_flags;

                pte_write(pte, new_pte_value);

                phys_addr += PAGE_SIZE;
                pte++;

                if (pte == end) {
                    if (phys_addr == phys_end) {
                        *offset_in = phys_addr - phys_begin;
                        return MAP_DONE;
                    }

                    walker->indices[0] = PGT_PTE_COUNT(1) - 1;
                    const bool should_fill_in =
                        walker->indices[1] != PGT_PTE_COUNT(2) - 1;

                    iterate_options.alloc_parents = should_fill_in;
                    iterate_options.alloc_level = should_fill_in;

                    ptwalker_result =
                        ptwalker_next_with_options(walker,
                                                   /*level=*/1,
                                                   &iterate_options);

                    if (__builtin_expect(ptwalker_result != E_PT_WALKER_OK, 0))
                    {
                        goto panic;
                    }

                    // Exit if the level above is at index 0, which may mean
                    // that a large page can be placed at the higher level.

                    if (!should_fill_in) {
                        *offset_in = phys_addr - phys_begin;
                        return MAP_RESTART;
                    }

                    break;
                } else if (phys_addr == phys_end) {
                    walker->indices[0] = pte - table;
                    *offset_in = phys_addr - phys_begin;

                    return MAP_DONE;
                }
            } while (true);
        } while (true);
    }
}

enum alloc_and_map_result {
    ALLOC_AND_MAP_DONE,

    ALLOC_AND_MAP_ALLOC_PAGE_FAIL,
    ALLOC_AND_MAP_ALLOC_PGTABLE_FAIL,

    ALLOC_AND_MAP_CONTINUE,
    ALLOC_AND_MAP_RESTART
};

__optimize(3) enum alloc_and_map_result
alloc_and_map_normal(struct pt_walker *const walker,
                     struct current_split_info *const curr_split,
                     struct pageop *const pageop,
                     const uint64_t virt_begin,
                     uint64_t *const offset_in,
                     const uint64_t size,
                     const struct pgmap_options *const options,
                     const struct pgmap_alloc_options *const alloc_options)
{
    uint64_t offset = *offset_in;
    if (__builtin_expect(offset >= size, 0)) {
        return ALLOC_AND_MAP_DONE;
    }

    enum pt_walker_result ptwalker_result = E_PT_WALKER_OK;
    struct pt_walker_iterate_options iterate_options = {
        .alloc_pgtable_cb_info = NULL,
        .free_pgtable_cb_info = NULL,

        .alloc_parents = false,
        .alloc_level = false,

        .should_ref = !options->is_in_early,
    };

    const uint64_t pte_flags = options->pte_flags;
    const pgmap_alloc_page_t alloc_a_page = alloc_options->alloc_page;

    if (options->is_overwrite) {
        do {
            ptwalker_result =
                ptwalker_fill_in_to(walker,
                                    /*level=*/1,
                                    /*should_ref=*/!options->is_in_early,
                                    options->alloc_pgtable_cb_info,
                                    options->free_pgtable_cb_info);

            if (__builtin_expect(ptwalker_result != E_PT_WALKER_OK, 0)) {
                return ALLOC_AND_MAP_ALLOC_PGTABLE_FAIL;
            }

            void *const alloc_page_cb_info = alloc_options->alloc_page_cb_info;
            do {
                const uint64_t page = alloc_a_page(alloc_page_cb_info);
                if (__builtin_expect(page == INVALID_PHYS, 0)) {
                    return ALLOC_AND_MAP_ALLOC_PAGE_FAIL;
                }

                const pte_t new_pte_value =
                    phys_create_pte(page) | PTE_LEAF_FLAGS | pte_flags;

                const enum override_result override_result =
                    override_pte(walker,
                                 curr_split,
                                 pageop,
                                 /*phys_begin=*/0,
                                 virt_begin,
                                 &offset,
                                 size,
                                 /*level=*/1,
                                 new_pte_value,
                                 options,
                                 /*is_alloc_mapping=*/true);

                switch (override_result) {
                    case OVERRIDE_OK:
                        break;
                    case OVERRIDE_DONE:
                        return ALLOC_AND_MAP_DONE;
                }

                const bool should_fill_in =
                    walker->indices[1] != PGT_PTE_COUNT(2) - 1;

                ptwalker_result =
                    ptwalker_next_with_options(walker,
                                               walker->level,
                                               &iterate_options);

                if (__builtin_expect(ptwalker_result != E_PT_WALKER_OK, 0)) {
                    return ALLOC_AND_MAP_ALLOC_PGTABLE_FAIL;
                }

                offset += PAGE_SIZE;
                if (offset == size) {
                    *offset_in = offset;
                    return ALLOC_AND_MAP_DONE;
                }

                if (walker->indices[0] == 0) {
                    // Exit if the level above is at index 0, which may mean
                    // that a large page can be placed at the higher level.

                    if (!should_fill_in) {
                        *offset_in = offset;
                        return ALLOC_AND_MAP_RESTART;
                    }

                    break;
                }
            } while (true);
        } while (true);
    } else {
        ptwalker_result =
            ptwalker_fill_in_to(walker,
                                /*level=*/1,
                                /*should_ref=*/!options->is_in_early,
                                options->alloc_pgtable_cb_info,
                                options->free_pgtable_cb_info);

        if (__builtin_expect(ptwalker_result != E_PT_WALKER_OK, 0)) {
            return ALLOC_AND_MAP_ALLOC_PGTABLE_FAIL;
        }

        void *const alloc_page_cb_info = alloc_options->alloc_page_cb_info;
        do {
            pte_t *const table = walker->tables[0];
            pte_t *pte = &table[walker->indices[0]];
            const pte_t *const end = &table[PGT_PTE_COUNT(1)];

            do {
                const uint64_t page = alloc_a_page(alloc_page_cb_info);
                if (__builtin_expect(page == INVALID_PHYS, 0)) {
                    return ALLOC_AND_MAP_ALLOC_PAGE_FAIL;
                }

                const pte_t new_pte_value =
                    phys_create_pte(page) | PTE_LEAF_FLAGS | pte_flags;

                pte_write(pte, new_pte_value);

                pte++;
                offset += PAGE_SIZE;

                if (pte == end) {
                    if (offset == size) {
                        *offset_in = offset;
                        return ALLOC_AND_MAP_DONE;
                    }

                    const bool should_fill_in =
                        walker->indices[1] != PGT_PTE_COUNT(2) - 1;

                    iterate_options.alloc_level = should_fill_in;
                    iterate_options.alloc_parents = should_fill_in;

                    walker->indices[0] = PGT_PTE_COUNT(1) - 1;
                    ptwalker_result =
                        ptwalker_next_with_options(walker,
                                                   walker->level,
                                                   &iterate_options);

                    if (__builtin_expect(ptwalker_result != E_PT_WALKER_OK, 0))
                    {
                        return ALLOC_AND_MAP_ALLOC_PGTABLE_FAIL;
                    }

                    // Exit if the level above is at index 0, which may mean
                    // that a large page can be placed at the higher level.

                    if (!should_fill_in) {
                        *offset_in = offset;
                        return ALLOC_AND_MAP_RESTART;
                    }

                    break;
                } else if (offset == size) {
                    walker->indices[0] = pte - table;
                    *offset_in = offset;

                    return ALLOC_AND_MAP_DONE;
                }
            } while (true);
        } while (true);
    }
}

__optimize(3) enum map_result
map_large_at_top_level_overwrite(struct pt_walker *const walker,
                                 struct current_split_info *const curr_split,
                                 struct pageop *const pageop,
                                 const uint64_t phys_begin,
                                 const uint64_t virt_begin,
                                 uint64_t *const offset_in,
                                 const uint64_t size,
                                 const struct pgmap_options *const options)
{
    const pgt_level_t level = walker->top_level;
    const struct pt_walker_iterate_options iterate_options = {
        .alloc_pgtable_cb_info = options->alloc_pgtable_cb_info,
        .free_pgtable_cb_info = options->free_pgtable_cb_info,

        .alloc_parents = false,
        .alloc_level = false,
        .should_ref = !options->is_in_early,
    };

    const uint64_t largepage_size = PAGE_SIZE_AT_LEVEL(level);
    const uint64_t pte_flags = options->pte_flags;

    uint64_t offset = *offset_in;
    do {
        const pte_t new_pte_value =
            phys_create_pte(phys_begin + offset) |
            PTE_LARGE_FLAGS(level) |
            pte_flags;

        const enum override_result override_result =
            override_pte(walker,
                         curr_split,
                         pageop,
                         phys_begin,
                         virt_begin,
                         offset_in,
                         size,
                         level,
                         new_pte_value,
                         options,
                         /*is_alloc_mapping=*/false);

        switch (override_result) {
            case OVERRIDE_OK:
                break;
            case OVERRIDE_DONE:
                return MAP_DONE;
        }

        const enum pt_walker_result ptwalker_result =
            ptwalker_next_with_options(walker, level, &iterate_options);

        if (__builtin_expect(ptwalker_result != E_PT_WALKER_OK, 0)) {
            panic("mm: failed to pgmap, result=%d\n", ptwalker_result);
        }

        offset += largepage_size;
        if (offset == size) {
            *offset_in = offset;
            return MAP_DONE;
        }
    } while (offset + largepage_size <= size);

    *offset_in = offset;
    return MAP_CONTINUE;
}

__optimize(3) enum map_result
map_large_at_top_level_no_overwrite(struct pt_walker *const walker,
                                    const uint64_t phys_begin,
                                    uint64_t *const offset_in,
                                    const uint64_t size,
                                    const struct pgmap_options *const options)
{
    const pgt_level_t level = walker->top_level;

    const uint64_t largepage_size = PAGE_SIZE_AT_LEVEL(level);
    const uint64_t phys_end = phys_begin + size;

    uint64_t offset = *offset_in;
    uint64_t phys_addr = phys_begin + offset;

    const uint64_t pte_flags = options->pte_flags;

    pte_t *const table = walker->tables[level - 1];
    pte_t *pte = &table[walker->indices[level - 1]];

    do {
        const pte_t new_pte_value =
            phys_create_pte(phys_addr) | PTE_LARGE_FLAGS(level) | pte_flags;

        pte_write(pte, new_pte_value);
        phys_addr += largepage_size;

        if (phys_addr == phys_end) {
            break;
        }

        pte++;
    } while (phys_addr + largepage_size <= phys_end);

    walker->indices[level - 1] = pte - table;
    *offset_in = phys_addr - phys_begin;

    return MAP_CONTINUE;
}

__optimize(3) enum map_result
map_large_at_level_overwrite(struct pt_walker *const walker,
                             struct current_split_info *const curr_split,
                             struct pageop *const pageop,
                             const uint64_t phys_begin,
                             const uint64_t virt_begin,
                             uint64_t *const offset_in,
                             const uint64_t size,
                             const pgt_level_t level,
                             const struct pgmap_options *const options)
{
    const bool should_ref = !options->is_in_early;
    enum pt_walker_result ptwalker_result =
        ptwalker_fill_in_to(walker,
                            level,
                            should_ref,
                            options->alloc_pgtable_cb_info,
                            options->free_pgtable_cb_info);

    if (__builtin_expect(ptwalker_result != E_PT_WALKER_OK, 0)) {
    panic:
        panic("mm: failed to pgmap, result=%d\n", ptwalker_result);
    }

    const struct pt_walker_iterate_options iterate_options = {
        .alloc_pgtable_cb_info = options->alloc_pgtable_cb_info,
        .free_pgtable_cb_info = options->free_pgtable_cb_info,

        .alloc_parents = false,
        .alloc_level = false,
        .should_ref = should_ref,
    };

    const uint64_t largepage_size = PAGE_SIZE_AT_LEVEL(level);
    const uint64_t pte_flags = options->pte_flags;

    uint64_t offset = *offset_in;
    do {
        const pte_t new_pte_value =
            phys_create_pte(phys_begin + offset) |
            PTE_LARGE_FLAGS(level) |
            pte_flags;

        const enum override_result override_result =
            override_pte(walker,
                         curr_split,
                         pageop,
                         phys_begin,
                         virt_begin,
                         offset_in,
                         size,
                         level,
                         new_pte_value,
                         options,
                         /*is_alloc_mapping=*/false);

        switch (override_result) {
            case OVERRIDE_OK:
                break;
            case OVERRIDE_DONE:
                return MAP_DONE;
        }

        ptwalker_result =
            ptwalker_next_with_options(walker, level, &iterate_options);

        if (__builtin_expect(ptwalker_result != E_PT_WALKER_OK, 0)) {
            goto panic;
        }

        offset += largepage_size;
        if (offset == size) {
            *offset_in = offset;
            return MAP_DONE;
        }

        if (walker->indices[walker->level - 1] == 0) {
            // Exit if the level above is at index 0, which may mean that a
            // large page can be placed at the higher level.

            if (walker->indices[walker->level] == 0) {
                *offset_in = offset;
                return MAP_RESTART;
            }

            break;
        }
    } while (offset + largepage_size <= size);

    *offset_in = offset;
    return MAP_CONTINUE;
}

__optimize(3) enum alloc_and_map_result
alloc_and_map_large_at_level_overwrite(
    struct pt_walker *const walker,
    struct current_split_info *const curr_split,
    struct pageop *const pageop,
    const uint64_t virt_begin,
    uint64_t *const offset_in,
    const uint64_t size,
    const pgt_level_t level,
    const struct pgmap_options *const options,
    const struct pgmap_alloc_options *const alloc_options)
{
    const bool should_ref = !options->is_in_early;
    enum pt_walker_result ptwalker_result =
        ptwalker_fill_in_to(walker,
                            level,
                            should_ref,
                            options->alloc_pgtable_cb_info,
                            options->free_pgtable_cb_info);

    if (__builtin_expect(ptwalker_result != E_PT_WALKER_OK, 0)) {
        return ALLOC_AND_MAP_ALLOC_PAGE_FAIL;
    }

    const struct pt_walker_iterate_options iterate_options = {
        .alloc_pgtable_cb_info = options->alloc_pgtable_cb_info,
        .free_pgtable_cb_info = options->free_pgtable_cb_info,

        .alloc_parents = false,
        .alloc_level = false,
        .should_ref = should_ref,
    };

    const uint64_t largepage_size = PAGE_SIZE_AT_LEVEL(level);
    const uint64_t pte_flags = options->pte_flags;

    const pgmap_alloc_large_page_t alloc_a_large_page =
        alloc_options->alloc_large_page;
    void *const alloc_large_page_cb_info =
        alloc_options->alloc_large_page_cb_info;

    uint64_t offset = *offset_in;
    do {
        const uint64_t page =
            alloc_a_large_page(level, alloc_large_page_cb_info);

        if (__builtin_expect(page == INVALID_PHYS, 0)) {
            return false;
        }

        const pte_t new_pte_value =
            phys_create_pte(page) | PTE_LARGE_FLAGS(level) | pte_flags;

        const enum override_result override_result =
            override_pte(walker,
                         curr_split,
                         pageop,
                         /*phys_begin=*/0,
                         virt_begin,
                         offset_in,
                         size,
                         level,
                         new_pte_value,
                         options,
                         /*is_alloc_mapping=*/false);

        switch (override_result) {
            case OVERRIDE_OK:
                break;
            case OVERRIDE_DONE:
                return ALLOC_AND_MAP_DONE;
        }

        ptwalker_result =
            ptwalker_next_with_options(walker, level, &iterate_options);

        if (__builtin_expect(ptwalker_result != E_PT_WALKER_OK, 0)) {
            return ALLOC_AND_MAP_ALLOC_PGTABLE_FAIL;
        }

        offset += largepage_size;
        if (offset == size) {
            *offset_in = offset;
            return ALLOC_AND_MAP_DONE;
        }

        if (walker->indices[walker->level - 1] == 0) {
            // Exit if the level above is at index 0, which may mean that a
            // large page can be placed at the higher level.

            if (walker->indices[walker->level] == 0) {
                *offset_in = offset;
                return ALLOC_AND_MAP_RESTART;
            }

            break;
        }
    } while (offset + largepage_size <= size);

    *offset_in = offset;
    return ALLOC_AND_MAP_CONTINUE;
}

__optimize(3) enum map_result
map_large_at_level_no_overwrite(struct pt_walker *const walker,
                                const uint64_t phys_begin,
                                uint64_t *const offset_in,
                                const uint64_t size,
                                const pgt_level_t level,
                                const struct pgmap_options *const options)
{
    const bool should_ref = !options->is_in_early;
    enum pt_walker_result ptwalker_result =
        ptwalker_fill_in_to(walker,
                            level,
                            should_ref,
                            options->alloc_pgtable_cb_info,
                            options->free_pgtable_cb_info);

    if (__builtin_expect(ptwalker_result != E_PT_WALKER_OK, 0)) {
    panic:
        panic("mm: failed to pgmap, result=%d\n", ptwalker_result);
    }

    const struct pt_walker_iterate_options iterate_options = {
        .alloc_pgtable_cb_info = options->alloc_pgtable_cb_info,
        .free_pgtable_cb_info = options->free_pgtable_cb_info,

        .alloc_parents = false,
        .alloc_level = false,
        .should_ref = should_ref,
    };

    const uint64_t phys_end = phys_begin + size;
    const uint64_t largepage_size = PAGE_SIZE_AT_LEVEL(level);
    const uint64_t pte_flags = options->pte_flags;

    uint64_t offset = *offset_in;
    uint64_t phys_addr = phys_begin + offset;

    do {
        pte_t *const table = walker->tables[level - 1];
        pte_t *pte = &table[walker->indices[level - 1]];
        const pte_t *const end = &table[PGT_PTE_COUNT(level)];

        do {
            const pte_t new_pte_value =
                phys_create_pte(phys_addr) | PTE_LARGE_FLAGS(level) | pte_flags;

            pte_write(pte, new_pte_value);
            phys_addr += largepage_size;

            if (phys_addr == phys_end) {
                walker->indices[level - 1] = pte - table;
                *offset_in = phys_addr - phys_begin;

                return MAP_DONE;
            }

            pte++;
            if (pte == end) {
                const bool should_fill_in =
                    walker->indices[level] != PGT_PTE_COUNT(level + 1) - 1;

                walker->indices[level - 1] = PGT_PTE_COUNT(level) - 1;
                ptwalker_result =
                    ptwalker_next_with_options(walker, level, &iterate_options);

                if (__builtin_expect(ptwalker_result != E_PT_WALKER_OK, 0)) {
                    goto panic;
                }

                // Exit if the level above is at index 0, which may mean that a
                // large page can be placed at the higher level.

                if (!should_fill_in) {
                    *offset_in = phys_addr - phys_begin;
                    return MAP_RESTART;
                }

                break;
            }

            if (phys_addr + largepage_size > phys_end) {
                walker->indices[level - 1] = pte - table;
                *offset_in = phys_addr - phys_begin;

                return MAP_CONTINUE;
            }
        } while (true);
    } while (true);
}

__optimize(3) enum alloc_and_map_result
alloc_and_map_large_at_level_no_overwrite(
    struct pt_walker *const walker,
    uint64_t *const offset_in,
    const uint64_t size,
    const pgt_level_t level,
    const struct pgmap_options *const options,
    const struct pgmap_alloc_options *const alloc_options)
{
    enum pt_walker_result ptwalker_result =
        ptwalker_fill_in_to(walker,
                            level,
                            !options->is_in_early,
                            options->alloc_pgtable_cb_info,
                            options->free_pgtable_cb_info);

    if (__builtin_expect(ptwalker_result != E_PT_WALKER_OK, 0)) {
        return ALLOC_AND_MAP_ALLOC_PGTABLE_FAIL;
    }

    struct pt_walker_iterate_options iterate_options = {
        .alloc_pgtable_cb_info = options->alloc_pgtable_cb_info,
        .free_pgtable_cb_info = options->free_pgtable_cb_info,

        .alloc_parents = false,
        .alloc_level = false,
        .should_ref = !options->is_in_early,
    };

    const uint64_t largepage_size = PAGE_SIZE_AT_LEVEL(level);
    const uint64_t pte_flags = options->pte_flags;

    const pgmap_alloc_large_page_t alloc_a_large_page =
        alloc_options->alloc_large_page;
    void *const alloc_large_page_cb_info =
        alloc_options->alloc_large_page_cb_info;

    uint64_t offset = *offset_in;
    do {
        pte_t *const table = walker->tables[level - 1];
        pte_t *pte = &table[walker->indices[level - 1]];
        const pte_t *const end = &table[PGT_PTE_COUNT(level)];

        do {
            const uint64_t page =
                alloc_a_large_page(level, alloc_large_page_cb_info);

            if (__builtin_expect(page == INVALID_PHYS, 0)) {
                return ALLOC_AND_MAP_ALLOC_PAGE_FAIL;
            }

            const pte_t new_pte_value =
                phys_create_pte(page) | PTE_LARGE_FLAGS(level) | pte_flags;

            pte_write(pte, new_pte_value);
            if (offset == size) {
                walker->indices[level - 1] = pte - table;
                *offset_in = offset;

                return ALLOC_AND_MAP_DONE;
            }

            pte++;
            if (pte == end) {
                const bool should_fill_in =
                    walker->indices[level] != PGT_PTE_COUNT(level + 1) - 1;

                iterate_options.alloc_parents = should_fill_in;
                iterate_options.alloc_level = should_fill_in;

                walker->indices[level - 1] = PGT_PTE_COUNT(level) - 1;
                ptwalker_result =
                    ptwalker_next_with_options(walker, level, &iterate_options);

                if (__builtin_expect(ptwalker_result != E_PT_WALKER_OK, 0)) {
                    return ALLOC_AND_MAP_ALLOC_PGTABLE_FAIL;
                }

                // Exit if the level above is at index 0, which may mean that a
                // large page can be placed at the higher level.

                if (!should_fill_in) {
                    *offset_in = offset;
                    return ALLOC_AND_MAP_RESTART;
                }

                break;
            }

            offset += largepage_size;
            if (offset + largepage_size > size) {
                walker->indices[level - 1] = pte - table;
                *offset_in = offset;

                return ALLOC_AND_MAP_CONTINUE;
            }
        } while (true);
    } while (true);
}

enum map_result
map_large_at_level(struct pt_walker *const walker,
                   struct current_split_info *const curr_split,
                   struct pageop *const pageop,
                   const uint64_t phys_begin,
                   const uint64_t virt_begin,
                   uint64_t *const offset_in,
                   const uint64_t size,
                   const pgt_level_t level,
                   const struct pgmap_options *const options)
{
    if (__builtin_expect(level != pgt_get_top_level(), 1)) {
        if (__builtin_expect(!options->is_overwrite, 1)) {
            return map_large_at_level_no_overwrite(walker,
                                                   phys_begin,
                                                   offset_in,
                                                   size,
                                                   level,
                                                   options);
        }

        return map_large_at_level_overwrite(walker,
                                            curr_split,
                                            pageop,
                                            phys_begin,
                                            virt_begin,
                                            offset_in,
                                            size,
                                            level,
                                            options);
    }

    if (!options->is_overwrite) {
        return map_large_at_top_level_no_overwrite(walker,
                                                   phys_begin,
                                                   offset_in,
                                                   size,
                                                   options);
    }

    return map_large_at_top_level_overwrite(walker,
                                            curr_split,
                                            pageop,
                                            phys_begin,
                                            virt_begin,
                                            offset_in,
                                            size,
                                            options);
}

static bool
pgmap_with_ptwalker(struct pt_walker *const walker,
                    struct current_split_info *const curr_split,
                    struct pageop *const pageop,
                    const struct range phys_range,
                    const uint64_t virt_begin,
                    const struct pgmap_options *const options)
{
    uint64_t offset = 0;
    const uint64_t supports_largepage_at_level_mask =
        options->supports_largepage_at_level_mask;

    do {
    start:
        if (supports_largepage_at_level_mask != 0) {
            // Find the largest page-size we can use.
            uint16_t highest_largepage_level = 0;
            for (pgt_level_t level = 1; level <= walker->top_level; level++) {
                if (walker->indices[level - 1] != 0) {
                    // If we don't have a zero at this level, but had one at all
                    // the preceding levels, then this present level is the
                    // highest largepage level.

                    highest_largepage_level = level;
                    break;
                }
            }

            if (highest_largepage_level > 1) {
                for (int16_t index = countof(LARGEPAGE_LEVELS) - 1;
                    index >= 0;
                    index--)
                {
                    const pgt_level_t level = LARGEPAGE_LEVELS[index];
                    if (level > highest_largepage_level) {
                        continue;
                    }

                    if ((supports_largepage_at_level_mask & 1ull << level) == 0)
                    {
                        continue;
                    }

                    const uint64_t largepage_size = PAGE_SIZE_AT_LEVEL(level);
                    if (!has_align(phys_range.front + offset, largepage_size) ||
                        offset + largepage_size > phys_range.size)
                    {
                        continue;
                    }

                    const enum map_result result =
                        map_large_at_level(walker,
                                           curr_split,
                                           pageop,
                                           phys_range.front,
                                           virt_begin,
                                           &offset,
                                           phys_range.size,
                                           level,
                                           options);

                    switch (result) {
                        case MAP_DONE:
                            finish_split_info(walker,
                                              curr_split,
                                              pageop,
                                              virt_begin + offset,
                                              options);
                            return true;
                        case MAP_CONTINUE:
                            break;
                        case MAP_RESTART:
                            goto start;
                    }
                }
            }
        }

        const enum map_result result =
            map_normal(walker,
                       curr_split,
                       pageop,
                       phys_range.front,
                       virt_begin,
                       &offset,
                       phys_range.size,
                       options);

        switch (result) {
            case MAP_DONE:
                finish_split_info(walker,
                                  curr_split,
                                  pageop,
                                  virt_begin + offset,
                                  options);
                return true;
            case MAP_CONTINUE:
            case MAP_RESTART:
                break;
        }
    } while (offset < phys_range.size);

    finish_split_info(walker, curr_split, pageop, virt_begin + offset, options);
    return true;
}

bool
pgmap_at(struct pagemap *const pagemap,
         struct range phys_range,
         uint64_t virt_addr,
         const struct pgmap_options *const options)
{
    if (__builtin_expect(range_empty(phys_range), 0)) {
        printk(LOGLEVEL_WARN, "mm: pgmap_at(): phys-range is empty\n");
        return false;
    }

    if (__builtin_expect(range_overflows(phys_range), 0)) {
        printk(LOGLEVEL_WARN,
               "mm: pgmap_at(): phys-range goes beyond end of address-space\n");
        return false;
    }

    if (__builtin_expect(!range_has_align(phys_range, PAGE_SIZE), 0)) {
        printk(LOGLEVEL_WARN,
               "mm: pgmap_at(): phys-range isn't aligned to PAGE_SIZE\n");
        return false;
    }

    const struct range virt_range = RANGE_INIT(virt_addr, phys_range.size);
    if (__builtin_expect(range_overflows(virt_range), 0)) {
        printk(LOGLEVEL_WARN,
               "mm: pgmap_at(): virt-range goes beyond end of address-space\n");
        return false;
    }

    if (__builtin_expect(!range_has_align(virt_range, PAGE_SIZE), 0)) {
        printk(LOGLEVEL_WARN,
               "mm: pgmap_at(): virt-range isn't aligned to PAGE_SIZE\n");
        return false;
    }

    struct pt_walker walker;
    if (options->is_in_early) {
        ptwalker_create_for_pagemap(&walker,
                                    pagemap,
                                    virt_addr,
                                    ptwalker_early_alloc_pgtable_cb,
                                    /*free_pgtable=*/NULL);
    } else {
        ptwalker_default_for_pagemap(&walker, pagemap, virt_addr);
    }

    /*
     * There's a chance the virtual address is pointing into the middle of a
     * large page, in which case we have to split the large page and
     * appropriately setup the current_split_info, but only if the large page
     * needs to be replaced (has a different phys-addr or different flags)
     */

    struct current_split_info curr_split = CURRENT_SPLIT_INFO_INIT();
    struct pageop pageop;

    if (options->is_overwrite) {
        pageop_init(&pageop, pagemap, virt_range);
    }

    if (options->is_overwrite && ptwalker_points_to_largepage(&walker)) {
        const uint64_t walker_virt_addr = ptwalker_get_virt_addr(&walker);
        if (walker_virt_addr < virt_addr) {
            uint64_t offset = virt_addr - walker_virt_addr;
            pte_t *const pte =
                walker.tables[walker.level - 1] +
                walker.indices[walker.level - 1];

            const pte_t entry = pte_read(pte);
            if (phys_range.front >= offset &&
                pte_to_phys(entry) == (phys_range.front - offset) &&
                pte_flags_equal(entry, walker.level, options->pte_flags))
            {
                offset =
                    walker_virt_addr +
                    PAGE_SIZE_AT_LEVEL(walker.level) -
                    virt_addr;

                if (!range_has_index(phys_range, offset)) {
                    return OVERRIDE_DONE;
                }

                phys_range = range_from_index(phys_range, offset);
                virt_addr += offset;
            } else {
                split_large_page(&walker,
                                 &pageop,
                                 &curr_split,
                                 walker.level,
                                 options);

                const struct range largepage_phys_range =
                    RANGE_INIT(curr_split.phys_range.front, offset);
                const bool result =
                    pgmap_with_ptwalker(&walker,
                                        /*curr_split=*/NULL,
                                        &pageop,
                                        largepage_phys_range,
                                        walker_virt_addr,
                                        options);

                if (!result) {
                    pageop_finish(&pageop);
                    return result;
                }

                curr_split.virt_addr = virt_addr;
                curr_split.phys_range =
                    range_from_index(curr_split.phys_range, offset);
            }
        }
    }

    const bool result =
        pgmap_with_ptwalker(&walker,
                            &curr_split,
                            &pageop,
                            phys_range,
                            virt_addr,
                            options);

    if (options->is_overwrite) {
        pageop_finish(&pageop);
    }

    return result;
}

static enum pgmap_alloc_result
pgmap_alloc_with_ptwalker(struct pt_walker *const walker,
                          struct current_split_info *const curr_split,
                          struct pageop *const pageop,
                          const struct range virt_range,
                          const struct pgmap_options *const options,
                          const struct pgmap_alloc_options *const alloc_options)
{
    uint64_t offset = 0;
    const uint64_t supports_largepage_at_level_mask =
        options->supports_largepage_at_level_mask;

    do {
    start:
        // Find the largest page-size we can use.
        uint16_t highest_largepage_level = 0;
        for (pgt_level_t level = 1; level <= walker->top_level; level++) {
            if (walker->indices[level - 1] != 0) {
                // If we don't have a zero at this level, but had one at all the
                // preceding levels, then this present level is the highest
                // largepage level.

                highest_largepage_level = level;
                break;
            }
        }

        if (highest_largepage_level > 1) {
            for (int16_t index = countof(LARGEPAGE_LEVELS) - 1;
                 index >= 0;
                 index--)
            {
                // Get index of level - 1 -> level - 2
                const pgt_level_t level = LARGEPAGE_LEVELS[index];
                if (level > highest_largepage_level) {
                    continue;
                }

                if ((supports_largepage_at_level_mask & (1ull << level)) == 0) {
                    continue;
                }

                const uint64_t largepage_size = PAGE_SIZE_AT_LEVEL(level);
                if (!has_align(virt_range.front + offset, largepage_size) ||
                    offset + largepage_size > virt_range.size)
                {
                    continue;
                }

                enum alloc_and_map_result result = ALLOC_AND_MAP_CONTINUE;
                if (options->is_overwrite) {
                    result =
                        alloc_and_map_large_at_level_overwrite(walker,
                                                               curr_split,
                                                               pageop,
                                                               virt_range.front,
                                                               &offset,
                                                               virt_range.size,
                                                               level,
                                                               options,
                                                               alloc_options);
                } else {
                    result =
                        alloc_and_map_large_at_level_no_overwrite(
                            walker,
                            &offset,
                            virt_range.size,
                            level,
                            options,
                            alloc_options);
                }

                switch (result) {
                    case ALLOC_AND_MAP_ALLOC_PAGE_FAIL:
                        return E_PGMAP_ALLOC_PAGE_ALLOC_FAIL;
                    case ALLOC_AND_MAP_ALLOC_PGTABLE_FAIL:
                        return E_PGMAP_ALLOC_PGTABLE_ALLOC_FAIL;
                    case ALLOC_AND_MAP_DONE:
                        finish_split_info(walker,
                                          curr_split,
                                          pageop,
                                          virt_range.front + offset,
                                          options);
                        return E_PGMAP_ALLOC_OK;
                    case ALLOC_AND_MAP_CONTINUE:
                        break;
                    case ALLOC_AND_MAP_RESTART:
                        goto start;
                }
            }
        }

        const enum alloc_and_map_result result =
            alloc_and_map_normal(walker,
                                 curr_split,
                                 pageop,
                                 virt_range.front,
                                 &offset,
                                 virt_range.size,
                                 options,
                                 alloc_options);

        switch (result) {
            case ALLOC_AND_MAP_ALLOC_PAGE_FAIL:
                return E_PGMAP_ALLOC_PAGE_ALLOC_FAIL;
            case ALLOC_AND_MAP_ALLOC_PGTABLE_FAIL:
                return E_PGMAP_ALLOC_PGTABLE_ALLOC_FAIL;
            case ALLOC_AND_MAP_DONE:
                finish_split_info(walker,
                                  curr_split,
                                  pageop,
                                  virt_range.front + offset,
                                  options);
                return E_PGMAP_ALLOC_OK;
            case ALLOC_AND_MAP_CONTINUE:
            case ALLOC_AND_MAP_RESTART:
                continue;
        }
    } while (false);

    finish_split_info(walker,
                      curr_split,
                      pageop,
                      virt_range.front + offset,
                      options);

    return E_PGMAP_ALLOC_OK;
}

enum pgmap_alloc_result
pgmap_alloc_at(struct pagemap *const pagemap,
               struct range virt_range,
               const struct pgmap_options *const options,
               const struct pgmap_alloc_options *const alloc_options)
{
    if (__builtin_expect(range_empty(virt_range), 0)) {
        printk(LOGLEVEL_WARN, "mm: pgmap_alloc_at(): virt-range is empty\n");
        return false;
    }

    if (__builtin_expect(range_overflows(virt_range), 0)) {
        printk(LOGLEVEL_WARN,
               "mm: pgmap_alloc_at(): virt-range goes beyond end of "
               "address-space\n");
        return false;
    }

    if (__builtin_expect(!range_has_align(virt_range, PAGE_SIZE), 0)) {
        printk(LOGLEVEL_WARN,
               "mm: pgmap_alloc_at(): virt-range isn't aligned to PAGE_SIZE\n");
        return false;
    }

    struct pt_walker walker;
    if (options->is_in_early) {
        ptwalker_create_for_pagemap(&walker,
                                    pagemap,
                                    virt_range.front,
                                    ptwalker_early_alloc_pgtable_cb,
                                    /*free_pgtable=*/NULL);
    } else {
        ptwalker_default_for_pagemap(&walker, pagemap, virt_range.front);
    }

    /*
     * There's a chance the virtual address is pointing into the middle of a
     * large page, in which case we have to split the large page and
     * appropriately setup the current_split_info, but only if the large page
     * needs to be replaced (has a different phys-addr or different flags)
     */

    struct current_split_info curr_split = CURRENT_SPLIT_INFO_INIT();
    struct pageop pageop;

    if (options->is_overwrite) {
        pageop_init(&pageop, pagemap, virt_range);
    }

    if (options->is_overwrite && ptwalker_points_to_largepage(&walker)) {
        const uint64_t walker_virt_addr = ptwalker_get_virt_addr(&walker);
        if (walker_virt_addr < virt_range.front) {
            const uint64_t offset = virt_range.front - walker_virt_addr;
            split_large_page(&walker,
                             &pageop,
                             &curr_split,
                             walker.level,
                             options);

            const struct range largepage_phys_range =
                RANGE_INIT(curr_split.phys_range.front, offset);
            const bool result =
                pgmap_with_ptwalker(&walker,
                                    /*curr_split=*/NULL,
                                    &pageop,
                                    largepage_phys_range,
                                    walker_virt_addr,
                                    options);

            if (!result) {
                pageop_finish(&pageop);
                return result;
            }

            curr_split.virt_addr = virt_range.front;
            curr_split.phys_range = range_create_upto(virt_range.size);
        }
    }

    const enum pgmap_alloc_result result =
        pgmap_alloc_with_ptwalker(&walker,
                                  &curr_split,
                                  &pageop,
                                  virt_range,
                                  options,
                                  alloc_options);

    if (options->is_overwrite) {
        pageop_finish(&pageop);
    }

    return result;
}

bool
pgunmap_at(struct pagemap *const pagemap,
           const struct range virt_range,
           const struct pgmap_options *const map_options,
           const struct pgunmap_options *const unmap_options)
{
    if (__builtin_expect(!range_has_align(virt_range, PAGE_SIZE), 0)) {
        printk(LOGLEVEL_WARN,
               "mm: pgunmap_at(): virt-range is not aligned to PAGE_SIZE\n");
        return false;
    }

    struct pt_walker walker;
    struct pageop pageop;

    ptwalker_default_for_pagemap(&walker, pagemap, virt_range.front);
    pageop_init(&pageop, pagemap, virt_range);

    const bool should_free_pages = unmap_options->free_pages;
    const bool dont_split_large_pages = unmap_options->dont_split_large_pages;

    // Try flushing entire tables if we can.
    pgt_level_t level = walker.level;
    for (pgt_level_t iter = level + 1; iter <= walker.top_level; iter++) {
        if (virt_range.size < PAGE_SIZE_AT_LEVEL(iter)) {
            level = iter - 1;
            break;
        }
    }

    uint64_t offset = 0;
    do {
        // Sanity check for the rare case where we have a bug in pgmap; a table
        // doesn't have a pte at the appropriate index, which we're supposed to
        // unmap.

        if (__builtin_expect(
                walker.level > 1 && !pte_level_can_have_large(walker.level), 0))
        {
            pageop_finish(&pageop);
            return false;
        }

        if (level < walker.level) {
            // Here, we're exclusively dealing with large pages that must be
            // split.

            if (dont_split_large_pages) {
                pageop_finish(&pageop);
                return false;
            }

            pte_t *const pte =
                &walker.tables[level - 1][walker.indices[level - 1]];

            // Sanity check for the rare case where we're not actually dealing
            // with a large page (instead, a bug in pgmap because we have a
            // table with a missing pte).

            const pte_t entry = pte_read(pte);
            if (__builtin_expect(!pte_is_large(entry), 0)) {
                pageop_finish(&pageop);
                return false;
            }

            pte_write(pte, /*value=*/0);
            ptwalker_deref_from_level(&walker, walker.level, &pageop);

            const uint64_t pte_phys = pte_to_phys(entry);
            if (pte_is_dirty(entry)) {
                set_pages_dirty(phys_to_page(pte_phys),
                                PAGE_SIZE_AT_LEVEL(walker.level) / PAGE_SIZE);
            }

            const uint64_t map_size = virt_range.size - offset;
            const bool map_result =
                pgmap_with_ptwalker(&walker,
                                    /*curr_split=*/NULL,
                                    &pageop,
                                    RANGE_INIT(pte_phys, map_size),
                                    virt_range.front + offset,
                                    map_options);

            if (!map_result) {
                pageop_finish(&pageop);
                return false;
            }
        } else {
            pte_t *const pte =
                &walker.tables[level - 1][walker.indices[level - 1]];

            if (should_free_pages) {
                const pte_t entry = pte_read(pte);

                pte_write(pte, /*value=*/0);
                pageop_flush_pte_in_current_range(&pageop,
                                                  entry,
                                                  level,
                                                  should_free_pages);
            } else {
                pte_write(pte, /*value=*/0);
            }

            ptwalker_deref_from_level(&walker, walker.level, &pageop);
        }

        const uint64_t page_size = PAGE_SIZE_AT_LEVEL(level);
        offset += page_size;

        if (offset == virt_range.size) {
            break;
        }

        walker.level = level;
        const enum pt_walker_result walker_result = ptwalker_next(&walker);

        if (__builtin_expect(walker_result != E_PT_WALKER_OK, 0)) {
            pageop_finish(&pageop);
            return false;
        }

        while (virt_range.size - offset < PAGE_SIZE_AT_LEVEL(level)) {
            level--;
        }
    } while (true);

    pageop_finish(&pageop);
    return true;
}