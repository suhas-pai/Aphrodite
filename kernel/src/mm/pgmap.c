/*
 * kernel/src/mm/pgmap.c
 * © suhas pai
 */

#include "asm/irqs.h"
#include "dev/printk.h"
#include "lib/align.h"

#include "page_alloc.h"
#include "pgmap.h"
#include "walker.h"

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
        RANGE_INIT(pte_to_phys(entry, level),
                   PAGE_SIZE_AT_LEVEL(walker->level));

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

__debug_optimize(3) static void
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
        if (options->free_pages) {
            deref_page(phys_to_page(curr_split->phys_range.front), pageop);
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

__unused static enum override_result
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
    if (!is_alloc_mapping
     && pte_to_phys(entry, level) == phys_addr
     && (pte_is_large(entry) ?
            pte_flags_equal(entry, walker->level, options->large_pte_flags)
          : pte_flags_equal(entry, walker->level, options->leaf_pte_flags)))
    {
        *offset_in += PAGE_SIZE_AT_LEVEL(walker->level);
        if (*offset_in >= size) {
            return OVERRIDE_DONE;
        }

        const struct ptwalker_iterate_options iterate_options = {
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

__debug_optimize(3)
static inline struct page *virt_to_table(const void *const ptr) {
    struct page *const page = virt_to_page(ptr);
    assert(page_get_state(page) == PAGE_STATE_TABLE);

    return page;
}

__debug_optimize(3) static inline uint64_t
get_leaf_pte_count_until_next_large(
    const struct range addr_range,
    const uint64_t addr,
    const uint64_t virt_begin,
    const bool check_virt,
    const pgt_level_t level,
    const uint64_t supports_largepage_at_level_mask)
{
    struct largepage_level_info *next_level_info = NULL;
    if (level != 1) {
        struct largepage_level_info *const info =
            &largepage_level_info_list[level - 1];

        carr_foreach_from_iter(largepage_level_info_list, jter, info + 1) {
            if (!jter->is_supported) {
                continue;
            }

            if ((supports_largepage_at_level_mask & 1ull << jter->level) == 0) {
                continue;
            }

            next_level_info = jter;
            break;
        }

        if (next_level_info == NULL) {
            return UINT64_MAX;
        }
    } else {
        next_level_info = &largepage_level_info_list[LARGEPAGE_LEVELS[0] - 1];

        bool next_level_info_valid = false;
        carr_foreach_from_iter(largepage_level_info_list, jter, next_level_info)
        {
            if (!jter->is_supported) {
                continue;
            }

            if ((supports_largepage_at_level_mask & 1ull << jter->level) == 0) {
                continue;
            }

            next_level_info = jter;
            next_level_info_valid = true;

            break;
        }

        if (!next_level_info_valid) {
            return UINT64_MAX;
        }
    }

    const uint64_t largepage_size = next_level_info->size;
    uint64_t largepage_addr = align_up_assert(addr, largepage_size);

    if (!range_has_end(addr_range, largepage_addr + largepage_size)) {
        return UINT64_MAX;
    }

    if (check_virt) {
        const uint64_t virt_addr = virt_begin + (addr - addr_range.front);
        const uint64_t largepage_virt_addr =
            virt_addr + amount_to_boundary(addr, largepage_size);

        if (!has_align(largepage_virt_addr, largepage_size)) {
            return UINT64_MAX;
        }

        uint64_t largepage_end = 0;
        if (!check_add(largepage_virt_addr, largepage_size, &largepage_end)) {
            return UINT64_MAX;
        }
    }

    return PAGE_COUNT(largepage_addr - addr);
}

__debug_optimize(3) static pgt_level_t
find_highest_possible_level(struct pt_walker *const walker,
                            const uint64_t supports_largepage_at_level_mask,
                            const struct range addr_range,
                            const uint64_t offset)
{
    // Find the largest page-size we can use.
    uint16_t highest_possible_level = 0;
    for (pgt_level_t level = 1; level <= walker->top_level; level++) {
        if (walker->indices[level - 1] != 0) {
            // If we don't have a zero at this level, but had one at all
            // the preceding levels, then this present level is the
            // highest largepage level.

            highest_possible_level = level;
            break;
        }
    }

    if (highest_possible_level == 1) {
        return highest_possible_level;
    }

    bool okay = false;
    struct largepage_level_info *const highest_large =
        &largepage_level_info_list[highest_possible_level - 1];

    carr_foreach_rev_from_iter(largepage_level_info_list, iter, highest_large) {
        const pgt_level_t level = iter->level;
        if (level == 1) {
            break;
        }

        if (!iter->is_supported) {
            continue;
        }

        if ((supports_largepage_at_level_mask & 1ull << level) == 0) {
            continue;
        }

        const uint64_t largepage_size = PAGE_SIZE_AT_LEVEL(level);
        if (!has_align(addr_range.front + offset, largepage_size)
         || offset + largepage_size > addr_range.size)
        {
            continue;
        }

        okay = true;
        highest_possible_level = level;

        break;
    }

    if (!okay) {
        return 1;
    }

    return highest_possible_level;
}

__debug_optimize(3) static inline uint64_t
write_ptes_at_level(
    struct pt_walker *const walker,
    pgt_level_t level,
    const uint64_t leaf_pte_flags,
    const uint64_t large_pte_flags,
    uint64_t phys_addr,
    uint64_t write_leaf_pte_count,
    const struct ptwalker_iterate_options *const iterate_options,
    uint64_t *const leaf_ptes_remaining_out)
{
    const uint64_t level_page_size = PAGE_SIZE_AT_LEVEL(level);
    const uint64_t leaf_pte_count_per_single_pte = PAGE_COUNT(level_page_size);

    uint64_t pte_flags = 0;
    if (level > 1) {
        pte_flags = large_pte_flags | PTE_LARGE_FLAGS(level);
    } else if (level == 1) {
        pte_flags = leaf_pte_flags | PTE_LEAF_FLAGS;
    } else {
        verify_not_reached();
    }

    uint64_t leaf_ptes_remaining = *leaf_ptes_remaining_out;
    uint64_t write_pte_count =
        write_leaf_pte_count / leaf_pte_count_per_single_pte;

    do {
        enum pt_walker_result result =
            ptwalker_fill_in_to(walker,
                                level,
                                iterate_options->should_ref,
                                iterate_options->alloc_pgtable_cb_info,
                                iterate_options->free_pgtable_cb_info);

        if (__builtin_expect(result != E_PT_WALKER_OK, 0)) {
            return UINT64_MAX;
        }

        pte_t *const table = walker->tables[level - 1];
        pte_t *const start = &table[walker->indices[level - 1]];

        pte_t *pte = start;

        const pte_t *const table_end = &table[PGT_PTE_COUNT(level)];
        const pte_t *const end = min(pte + write_pte_count, table_end);

        for (; pte != end; pte++) {
            pte_write(pte, phys_create_pte(phys_addr) | pte_flags);
            phys_addr += level_page_size;
        }

        const uint16_t count = pte - start;
        if (iterate_options->should_ref) {
            refcount_increment(&virt_to_table(table)->table.refcount, count);
        }

        walker->indices[level - 1] += count - 1;
        result = ptwalker_next_with_options(walker, level, iterate_options);

        if (__builtin_expect(result != E_PT_WALKER_OK, 0)) {
            return UINT64_MAX;
        }

        write_pte_count -= count;
        leaf_ptes_remaining -=
            check_mul_assert(leaf_pte_count_per_single_pte, count);
    } while (write_pte_count != 0);

    *leaf_ptes_remaining_out = leaf_ptes_remaining;
    return phys_addr;
}

__debug_optimize(3) static inline uint64_t
write_ptes_down_from_level(
    struct pt_walker *const walker,
    pgt_level_t level,
    const uint64_t leaf_pte_flags,
    const uint64_t large_pte_flags,
    uint64_t phys_addr,
    uint64_t leaf_ptes_remaining,
    const uint64_t supports_largepage_at_level_mask,
    const struct ptwalker_iterate_options *const iterate_options)
{
    phys_addr =
        write_ptes_at_level(walker,
                            level,
                            leaf_pte_flags,
                            large_pte_flags,
                            phys_addr,
                            leaf_ptes_remaining,
                            iterate_options,
                            &leaf_ptes_remaining);

    if (__builtin_expect(phys_addr == UINT64_MAX, 0)) {
        return phys_addr;
    }

    if (leaf_ptes_remaining == 0) {
        return phys_addr;
    }

    const uint64_t size_remaining =
        check_mul_assert(leaf_ptes_remaining, PAGE_SIZE);

    do {
        while (true) {
            level--;
            if (level == 1) {
                break;
            }

            if (!largepage_level_info_list[level - 1].is_supported) {
                continue;
            }

            if ((supports_largepage_at_level_mask & 1ull << level) == 0) {
                continue;
            }

            if (size_remaining < PAGE_SIZE_AT_LEVEL(level)) {
                continue;
            }

            break;
        }

        phys_addr =
            write_ptes_at_level(walker,
                                level,
                                leaf_pte_flags,
                                large_pte_flags,
                                phys_addr,
                                leaf_ptes_remaining,
                                iterate_options,
                                &leaf_ptes_remaining);

        if (__builtin_expect(phys_addr == UINT64_MAX, 0)) {
            break;
        }
    } while (leaf_ptes_remaining != 0);

    return phys_addr;
}

static bool
pgmap_with_ptwalker(struct pt_walker *const walker,
                    struct current_split_info *const curr_split,
                    struct pageop *const pageop,
                    const struct range phys_range,
                    const uint64_t virt_begin,
                    const struct pgmap_options *const options)
{
    const uint64_t supports_largepage_at_level_mask =
        options->supports_largepage_at_level_mask;

    uint64_t phys_addr = phys_range.front;
    uint64_t total_remaining_leaf_pte = PAGE_COUNT(phys_range.size);

    const struct ptwalker_iterate_options iterate_options = {
        .alloc_pgtable_cb_info = NULL,
        .free_pgtable_cb_info = NULL,

        .alloc_parents = false,
        .alloc_level = false,
        .should_ref = !options->is_in_early,
    };

    pgt_level_t level = 1;
    do {
        level =
            find_highest_possible_level(walker,
                                        supports_largepage_at_level_mask,
                                        phys_range,
                                        (phys_addr - phys_range.front));

        const uint64_t leaf_pte_count_until_next_large =
            get_leaf_pte_count_until_next_large(
                phys_range,
                phys_addr,
                virt_begin,
                /*check_virt=*/true,
                level,
                supports_largepage_at_level_mask);

        if (total_remaining_leaf_pte <= leaf_pte_count_until_next_large) {
            break;
        }

        phys_addr =
            write_ptes_at_level(walker,
                                level,
                                options->leaf_pte_flags,
                                options->large_pte_flags,
                                phys_addr,
                                leaf_pte_count_until_next_large,
                                &iterate_options,
                                &total_remaining_leaf_pte);

        if (__builtin_expect(phys_addr == UINT64_MAX, 0)) {
            return false;
        }
    } while (true);

    phys_addr =
        write_ptes_down_from_level(walker,
                                   level,
                                   options->leaf_pte_flags,
                                   options->large_pte_flags,
                                   phys_addr,
                                   total_remaining_leaf_pte,
                                   supports_largepage_at_level_mask,
                                   &iterate_options);

    if (__builtin_expect(phys_addr == UINT64_MAX, 0)) {
        return false;
    }

    finish_split_info(walker,
                      curr_split,
                      pageop,
                      virt_begin + (phys_addr - phys_range.front),
                      options);
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

    if (__builtin_expect(!has_align(virt_addr, PAGE_SIZE), 0)) {
        printk(LOGLEVEL_WARN,
               "mm: pgmap_at(): virt-addr isn't aligned to PAGE_SIZE\n");
        return false;
    }

    const bool flag = disable_irqs_if_enabled();
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

    struct current_split_info curr_split = CURRENT_SPLIT_INFO_INIT();
    struct pageop pageop;

    if (options->is_overwrite) {
        pageop_init(&pageop, pagemap, virt_range);
    }

    /*
     * There's a chance the virtual address is pointing into the middle of a
     * large page, in which case we have to split the large page and
     * appropriately setup the current_split_info, but only if the large page
     * needs to be replaced (has a different phys-addr or different flags).
     */

    if (options->is_overwrite && ptwalker_points_to_largepage(&walker)) {
        const uint64_t walker_virt_addr = ptwalker_get_virt_addr(&walker);
        if (walker_virt_addr < virt_addr) {
            uint64_t offset = virt_addr - walker_virt_addr;
            pte_t *const pte =
                walker.tables[walker.level - 1]
              + walker.indices[walker.level - 1];

            const pte_t entry = pte_read(pte);
            if (phys_range.front >= offset
             && pte_to_phys(entry, walker.level) == (phys_range.front - offset)
             && pte_flags_equal(entry, walker.level, options->leaf_pte_flags))
            {
                offset =
                    walker_virt_addr + PAGE_SIZE_AT_LEVEL(walker.level)
                  - virt_addr;

                if (!range_has_index(phys_range, offset)) {
                    enable_irqs_if_flag(flag);
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
                    enable_irqs_if_flag(flag);

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

    enable_irqs_if_flag(flag);
    return result;
}

__debug_optimize(3) static inline enum pgmap_alloc_result
alloc_ptes_at_level(
    struct pt_walker *const walker,
    pgt_level_t level,
    const uint64_t leaf_pte_flags,
    const uint64_t large_pte_flags,
    const uint64_t write_leaf_pte_count,
    const struct pgmap_alloc_options *const alloc_options,
    const struct ptwalker_iterate_options *const iterate_options,
    uint64_t *const leaf_ptes_remaining_out)
{
    const uint64_t leaf_pte_count_per_single_pte =
        PAGE_COUNT(PAGE_SIZE_AT_LEVEL(level));

    uint64_t pte_flags = 0;
    if (level > 1) {
        pte_flags = large_pte_flags | PTE_LARGE_FLAGS(level);
    } else if (level == 1) {
        pte_flags = leaf_pte_flags | PTE_LEAF_FLAGS;
    } else {
        verify_not_reached();
    }

    const pgmap_alloc_page_t alloc_a_page = alloc_options->alloc_page;
    void *const alloc_page_cb_info = alloc_options->alloc_page_cb_info;

    const pgmap_alloc_large_page_t alloc_a_large_page =
        alloc_options->alloc_large_page;
    void *const alloc_large_page_cb_info =
        alloc_options->alloc_large_page_cb_info;

    uint64_t leaf_ptes_remaining = *leaf_ptes_remaining_out;
    uint64_t write_pte_count =
        write_leaf_pte_count / leaf_pte_count_per_single_pte;

    do {
        enum pt_walker_result result =
            ptwalker_fill_in_to(walker,
                                level,
                                iterate_options->should_ref,
                                iterate_options->alloc_pgtable_cb_info,
                                iterate_options->free_pgtable_cb_info);

        if (__builtin_expect(result != E_PT_WALKER_OK, 0)) {
            return E_PGMAP_ALLOC_PGTABLE_ALLOC_FAIL;
        }

        pte_t *const table = walker->tables[level - 1];
        pte_t *const start = &table[walker->indices[level - 1]];

        pte_t *pte = start;

        const pte_t *const table_end = &table[PGT_PTE_COUNT(level)];
        const pte_t *const end = min(pte + write_pte_count, table_end);

        if (level > 1) {
            for (; pte != end; pte++) {
                const uint64_t phys =
                    alloc_a_large_page(level, alloc_large_page_cb_info);

                if (phys == INVALID_PHYS) {
                    return E_PGMAP_ALLOC_PAGE_ALLOC_FAIL;
                }

                pte_write(pte, phys_create_pte(phys) | pte_flags);
            }
        } else {
            for (; pte != end; pte++) {
                const uint64_t phys = alloc_a_page(alloc_page_cb_info);
                if (__builtin_expect(phys == INVALID_PHYS, 0)) {
                    return E_PGMAP_ALLOC_PAGE_ALLOC_FAIL;
                }

                pte_write(pte, phys_create_pte(phys) | pte_flags);
            }
        }

        const uint16_t count = pte - start;
        if (iterate_options->should_ref) {
            refcount_increment(&virt_to_table(table)->table.refcount, count);
        }

        walker->indices[level - 1] += count - 1;
        result = ptwalker_next_with_options(walker, level, iterate_options);

        if (__builtin_expect(result != E_PT_WALKER_OK, 0)) {
            return E_PGMAP_ALLOC_PGTABLE_ALLOC_FAIL;
        }

        write_pte_count -= count;
        leaf_ptes_remaining -=
            check_mul_assert(leaf_pte_count_per_single_pte, count);
    } while (write_pte_count != 0);

    *leaf_ptes_remaining_out = leaf_ptes_remaining;
    return E_PGMAP_ALLOC_OK;
}

__debug_optimize(3) static inline bool
alloc_ptes_down_from_level(
    struct pt_walker *const walker,
    pgt_level_t level,
    const uint64_t leaf_pte_flags,
    const uint64_t large_pte_flags,
    uint64_t leaf_ptes_remaining,
    const uint64_t supports_largepage_at_level_mask,
    const struct pgmap_alloc_options *const alloc_options,
    const struct ptwalker_iterate_options *const iterate_options)
{
    enum pgmap_alloc_result result =
        alloc_ptes_at_level(walker,
                            level,
                            leaf_pte_flags,
                            large_pte_flags,
                            leaf_ptes_remaining,
                            alloc_options,
                            iterate_options,
                            &leaf_ptes_remaining);

    if (result != E_PGMAP_ALLOC_OK) {
        return result;
    }

    if (leaf_ptes_remaining == 0) {
        return E_PGMAP_ALLOC_OK;
    }

    const uint64_t size_remaining =
        check_mul_assert(leaf_ptes_remaining, PAGE_SIZE);

    do {
        while (true) {
            level--;
            if (level == 1) {
                break;
            }

            if (!largepage_level_info_list[level - 1].is_supported) {
                continue;
            }

            if ((supports_largepage_at_level_mask & 1ull << level) == 0) {
                continue;
            }

            if (size_remaining < PAGE_SIZE_AT_LEVEL(level)) {
                continue;
            }

            break;
        }

        result =
            alloc_ptes_at_level(walker,
                                level,
                                leaf_pte_flags,
                                large_pte_flags,
                                leaf_ptes_remaining,
                                alloc_options,
                                iterate_options,
                                &leaf_ptes_remaining);

        if (result != E_PGMAP_ALLOC_OK) {
            return result;
        }
    } while (leaf_ptes_remaining != 0);

    return E_PGMAP_ALLOC_OK;
}

static enum pgmap_alloc_result
pgmap_alloc_with_ptwalker(struct pt_walker *const walker,
                          struct current_split_info *const curr_split,
                          struct pageop *const pageop,
                          const struct range virt_range,
                          const struct pgmap_options *const options,
                          const struct pgmap_alloc_options *const alloc_options)
{
    const uint64_t supports_largepage_at_level_mask =
        options->supports_largepage_at_level_mask;

    const struct ptwalker_iterate_options iterate_options = {
        .alloc_pgtable_cb_info = NULL,
        .free_pgtable_cb_info = NULL,

        .alloc_parents = false,
        .alloc_level = false,
        .should_ref = !options->is_in_early,
    };

    uint64_t total_remaining_leaf_pte = PAGE_COUNT(virt_range.size);
    uint64_t virt_addr = virt_range.front;

    pgt_level_t highest_possible_level = 1;
    do {
        highest_possible_level =
            find_highest_possible_level(walker,
                                        supports_largepage_at_level_mask,
                                        virt_range,
                                        (virt_addr - virt_range.front));

        const uint64_t leaf_pte_count_until_next_large =
            get_leaf_pte_count_until_next_large(
                virt_range,
                virt_addr,
                /*virt_begin=*/0,
                /*check_virt=*/false,
                highest_possible_level,
                supports_largepage_at_level_mask);

        if (total_remaining_leaf_pte <= leaf_pte_count_until_next_large) {
            break;
        }

        const uint64_t old_total_remaining_leaf_pte = total_remaining_leaf_pte;
        const enum pgmap_alloc_result result =
            alloc_ptes_at_level(walker,
                                highest_possible_level,
                                options->leaf_pte_flags,
                                options->large_pte_flags,
                                leaf_pte_count_until_next_large,
                                alloc_options,
                                &iterate_options,
                                &total_remaining_leaf_pte);

        if (result != E_PGMAP_ALLOC_OK) {
            return result;
        }

        virt_addr +=
            (old_total_remaining_leaf_pte - total_remaining_leaf_pte)
            * PAGE_SIZE;
    } while (true);

    const enum pgmap_alloc_result result =
        alloc_ptes_down_from_level(walker,
                                   highest_possible_level,
                                   options->leaf_pte_flags,
                                   options->large_pte_flags,
                                   total_remaining_leaf_pte,
                                   supports_largepage_at_level_mask,
                                   alloc_options,
                                   &iterate_options);

    if (result != E_PGMAP_ALLOC_OK) {
        return result;
    }

    finish_split_info(walker,
                      curr_split,
                      pageop,
                      range_get_end_assert(virt_range),
                      options);

    return E_PGMAP_ALLOC_OK;
}

static inline bool
split_initial_large_page_if_necessary(
    struct pt_walker *const walker,
    struct pageop *const pageop,
    struct current_split_info *const curr_split,
    struct range *const virt_range_in,
    const struct pgmap_options *const options,
    const bool calculate_phys)
{
    const uint64_t walker_virt_addr = ptwalker_get_virt_addr(walker);
    const struct range virt_range = *virt_range_in;

    if (walker_virt_addr >= virt_range.front) {
        return true;
    }

    const uint64_t offset = virt_range.front - walker_virt_addr;
    split_large_page(walker,
                     pageop,
                     curr_split,
                     walker->level,
                     options);

    uint64_t phys_front = curr_split->phys_range.front;
    if (calculate_phys) {
        phys_front = ptwalker_get_phys_addr(walker);
        assert(phys_front != INVALID_PHYS);
    }

    const struct range largepage_phys_range = RANGE_INIT(phys_front, offset);
    const bool result =
        pgmap_with_ptwalker(walker,
                            /*curr_split=*/NULL,
                            pageop,
                            largepage_phys_range,
                            walker_virt_addr,
                            options);

    if (!result) {
        return result;
    }

    curr_split->virt_addr = virt_range.front;
    curr_split->phys_range = range_from_index(curr_split->phys_range, offset);

    *virt_range_in = range_from_index(virt_range, offset);
    return true;
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

    const bool flag = disable_irqs_if_enabled();
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
        if (!split_initial_large_page_if_necessary(&walker,
                                                   &pageop,
                                                   &curr_split,
                                                   &virt_range,
                                                   options,
                                                   /*calculate_phys=*/false))
        {
            pageop_finish(&pageop);
            enable_irqs_if_flag(flag);

            return E_PGMAP_ALLOC_PGTABLE_ALLOC_FAIL;
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

    enable_irqs_if_flag(flag);
    return result;
}

bool
pgunmap_at(struct pagemap *const pagemap,
           struct range virt_range,
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

    const bool flag = disable_irqs_if_enabled();

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

    // Split a large page if we're pointing within one.
    struct current_split_info curr_split = CURRENT_SPLIT_INFO_INIT();
    if (!split_initial_large_page_if_necessary(&walker,
                                               &pageop,
                                               &curr_split,
                                               &virt_range,
                                               map_options,
                                               /*calculate_phys=*/true))
    {
        pageop_finish(&pageop);
        enable_irqs_if_flag(flag);

        printk(LOGLEVEL_WARN, "mm: pgunmap_at() failed to split large page\n");
        return false;
    }

    uint64_t remaining = virt_range.size;
    do {
        // Sanity check for the rare case where we have a bug in pgmap; a table
        // doesn't have a pte at the appropriate index, which we're supposed to
        // unmap.

        if (__builtin_expect(
                walker.level > 1 && !pte_level_can_have_large(walker.level), 0))
        {
            pageop_finish(&pageop);
            enable_irqs_if_flag(flag);

            printk(LOGLEVEL_WARN,
                   "mm: pgunmap_at() encountered a table with a missing "
                   "entry\n");

            return false;
        }

        if (level < walker.level) {
            // Here, we're exclusively dealing with large pages that may need to
            // be split.

            if (dont_split_large_pages) {
                pageop_finish(&pageop);
                enable_irqs_if_flag(flag);

                return false;
            }

            pte_t *const pte =
                &walker.tables[level - 1][walker.indices[level - 1]];

            // Sanity check for the rare case where we're not actually dealing
            // with a large page.

            const pte_t entry = pte_read(pte);
            if (__builtin_expect(!pte_is_large(entry), 0)) {
                pageop_finish(&pageop);
                enable_irqs_if_flag(flag);

                printk(LOGLEVEL_WARN,
                       "mm: pgunmap_at() encountered a pte that isn't large "
                       "when it should be, at level %" PRIu8 "\n",
                       level);

                return false;
            }

            pte_write(pte, /*value=*/0);

            const uint64_t pte_phys = pte_to_phys(entry, level);
            if (pte_is_dirty(entry)) {
                set_pages_dirty(phys_to_page(pte_phys),
                                PAGE_COUNT(PAGE_SIZE_AT_LEVEL(level)));
            }

            ptwalker_deref_from_level(&walker, level, &pageop);

            const uint64_t map_size = remaining;
            const bool map_result =
                pgmap_with_ptwalker(&walker,
                                    /*curr_split=*/NULL,
                                    &pageop,
                                    RANGE_INIT(pte_phys, map_size),
                                    virt_range.front
                                        + (virt_range.size - remaining),
                                    map_options);

            if (!map_result) {
                pageop_finish(&pageop);
                enable_irqs_if_flag(flag);

                return false;
            }

            break;
        }

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
        remaining -= PAGE_SIZE_AT_LEVEL(level);

        if (remaining == 0) {
            break;
        }

        walker.level = level;
        const enum pt_walker_result walker_result = ptwalker_next(&walker);

        if (__builtin_expect(walker_result != E_PT_WALKER_OK, 0)) {
            pageop_finish(&pageop);
            enable_irqs_if_flag(flag);

            return false;
        }

        while (remaining < PAGE_SIZE_AT_LEVEL(level)) {
            level--;
        }
    } while (true);

    pageop_finish(&pageop);
    enable_irqs_if_flag(flag);

    return true;
}