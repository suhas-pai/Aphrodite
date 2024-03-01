/*
 * kernel/src/mm/walker.c
 * © suhas pai
 */

#include "cpu/info.h"
#include "lib/align.h"

#include "lib/util.h"
#include "sched/process.h"

#include "early.h"
#include "page_alloc.h"
#include "walker.h"

__optimize(3) static uint64_t
ptwalker_alloc_pgtable_cb(struct pt_walker *const walker,
                          const pgt_level_t level,
                          void *const cb_info)
{
    (void)walker;
    (void)level;
    (void)cb_info;

    struct page *const table = alloc_table();
    if (table != NULL) {
        return page_to_phys(table);
    }

    return INVALID_PHYS;
}

__optimize(3) static void
ptwalker_free_pgtable_cb(struct pt_walker *const walker,
                         struct page *const page,
                         void *const cb_info)
{
    (void)walker;

    struct pageop *const pageop = (struct pageop *)cb_info;
    list_add(&pageop->delayed_free, &page->table.delayed_free_list);
}

__optimize(3) uint64_t
ptwalker_early_alloc_pgtable_cb(struct pt_walker *const walker,
                                const pgt_level_t level,
                                void *const cb_info)
{
    (void)walker;
    (void)level;
    (void)cb_info;

    const uint64_t phys = early_alloc_page();
    if (__builtin_expect(phys != INVALID_PHYS, 1)) {
        return phys;
    }

    panic("mm: failed to setup page-structs, ran out of memory\n");
}

static inline uint64_t
get_root_phys(const struct pagemap *const pagemap, const uint64_t virt_addr) {
#if defined(__aarch64__)
    uint64_t root_phys = 0;
    if (virt_addr & (1ull << 63)) {
        root_phys = virt_to_phys(pagemap->higher_root);
    } else {
        root_phys = virt_to_phys(pagemap->lower_root);
    }
#else
    (void)virt_addr;
    const uint64_t root_phys = virt_to_phys(pagemap->root);
#endif /* defined(__x86_64__) */

    return root_phys;
}

void
ptwalker_default(struct pt_walker *const walker, const uint64_t virt_addr) {
    return ptwalker_default_for_pagemap(walker,
                                        &this_cpu()->process->pagemap,
                                        virt_addr);
}

void
ptwalker_default_for_pagemap(struct pt_walker *const walker,
                             const struct pagemap *const pagemap,
                             const uint64_t virt_addr)
{
    return ptwalker_create_for_pagemap(walker,
                                       pagemap,
                                       virt_addr,
                                       ptwalker_alloc_pgtable_cb,
                                       ptwalker_free_pgtable_cb);
}

void
ptwalker_create(struct pt_walker *const walker,
                const uint64_t virt_addr,
                const ptwalker_alloc_pgtable_t alloc_pgtable,
                const ptwalker_free_pgtable_t free_pgtable)
{
    return ptwalker_create_for_pagemap(walker,
                                       &this_cpu()->process->pagemap,
                                       virt_addr,
                                       alloc_pgtable,
                                       free_pgtable);
}

void
ptwalker_create_for_pagemap(struct pt_walker *const walker,
                            const struct pagemap *const pagemap,
                            const uint64_t virt_addr,
                            const ptwalker_alloc_pgtable_t alloc_pgtable,
                            const ptwalker_free_pgtable_t free_pgtable)
{
    ptwalker_create_from_root_phys(walker,
                                   get_root_phys(pagemap, virt_addr),
                                   virt_addr,
                                   alloc_pgtable,
                                   free_pgtable);
}

void
ptwalker_create_from_root_phys(struct pt_walker *const walker,
                               const uint64_t root_phys,
                               const uint64_t virt_addr,
                               const ptwalker_alloc_pgtable_t alloc_pgtable,
                               const ptwalker_free_pgtable_t free_pgtable)
{
    assert(has_align(root_phys, PAGE_SIZE));
    assert(has_align(virt_addr, PAGE_SIZE));

    walker->level = pgt_get_top_level();
    walker->top_level = walker->level;

    walker->alloc_pgtable = alloc_pgtable;
    walker->free_pgtable = free_pgtable;

    walker->tables[walker->top_level - 1] = phys_to_virt(root_phys);
    walker->indices[walker->top_level - 1] =
        virt_to_pt_index(virt_addr, walker->top_level);

    pte_t *prev_table = walker->tables[walker->top_level - 1];
    for (pgt_level_t level = walker->top_level - 1; level >= 1; level--) {
        pte_t *table = NULL;
        if (prev_table != NULL) {
            const pgt_level_t parent_level = level + 1;
            const pgt_index_t index = walker->indices[parent_level - 1];

            const pte_t entry = pte_read(&prev_table[index]);
            if (pte_is_present(entry)) {
                if (!pte_level_can_have_large(parent_level) ||
                    !pte_is_large(entry))
                {
                    table = pte_to_virt(entry);
                    walker->level = level;
                } else {
                    walker->level = parent_level;
                }
            }
        }

        walker->tables[level - 1] = table;
        walker->indices[level - 1] = virt_to_pt_index(virt_addr, level);

        prev_table = table;
    }

    for (pgt_level_t index = walker->top_level;
         index != PGT_LEVEL_COUNT;
         index++)
    {
        walker->tables[index] = NULL;
        walker->indices[index] = 0;
    }
}

void
ptwalker_create_from_toplevel(struct pt_walker *const walker,
                              const uint64_t root_phys,
                              const pgt_level_t top_level,
                              const pgt_index_t root_index,
                              const ptwalker_alloc_pgtable_t alloc_pgtable,
                              const ptwalker_free_pgtable_t free_pgtable)
{
    walker->level = top_level;
    walker->top_level = walker->level;

    walker->alloc_pgtable = alloc_pgtable;
    walker->free_pgtable = free_pgtable;

    bzero(walker->tables, sizeof(walker->tables));
    bzero(walker->indices, sizeof(walker->indices));

    walker->tables[walker->top_level - 1] = phys_to_virt(root_phys);
    walker->indices[walker->top_level - 1] = root_index;
}

static const struct pt_walker_iterate_options default_options = {
    .alloc_pgtable_cb_info = NULL,
    .free_pgtable_cb_info = NULL,

    .alloc_parents = true,
    .alloc_level = true,
    .should_ref = true,
};

__optimize(3)
enum pt_walker_result ptwalker_next(struct pt_walker *const walker) {
    return ptwalker_next_with_options(walker, walker->level, &default_options);
}

__optimize(3) static void
reset_levels_lower_than(struct pt_walker *const walker, pgt_level_t level) {
    for (level--; level >= 1; level--) {
        walker->tables[level - 1] = NULL;
        walker->indices[level - 1] = 0;
    }
}

__optimize(3) static void
setup_levels_lower_than(struct pt_walker *const walker,
                        const pgt_level_t parent_level,
                        pte_t *const first_pte,
                        const pte_t first_entry)
{
    pgt_level_t level = parent_level - 1;

    pte_t *pte = first_pte;
    pte_t entry = first_entry;

    while (true) {
        pte_t *const table = pte_to_virt(entry);

        walker->tables[level - 1] = table;
        walker->indices[level - 1] = 0;

        pte = table;
        entry = pte_read(pte);

        if (!pte_is_present(entry) ||
            (pte_level_can_have_large(level) && pte_is_large(entry)))
        {
            walker->level = level;
            break;
        }

        level--;
        if (level < 1) {
            walker->level = 1;
            break;
        }
    }
}

__optimize(3) static void
ptwalker_drop_lowest(struct pt_walker *const walker,
                     void *const free_pgtable_cb_info)
{
    walker->tables[walker->level - 1] = NULL;
    walker->indices[walker->level - 1] = 0;

    ptwalker_deref_from_level(walker, walker->level + 1, free_pgtable_cb_info);
}

__optimize(3) static inline bool
alloc_table_at_pte(struct pt_walker *const walker,
                   pte_t *const pte_in_parent,
                   const pgt_level_t level,
                   void *const alloc_pgtable_cb_info)
{
    const uint64_t phys =
        walker->alloc_pgtable(walker, level, alloc_pgtable_cb_info);

    if (phys == INVALID_PHYS) {
        return false;
    }

    walker->tables[level - 1] = phys_to_virt(phys);
    pte_write(pte_in_parent, phys_create_pte(phys) | PGT_FLAGS);

    return true;
}

__optimize(3) static enum pt_walker_result
alloc_levels_down_to(struct pt_walker *const walker,
                     const pgt_level_t parent_level,
                     const pgt_level_t last_level,
                     const bool alloc_last_level,
                     const bool should_ref,
                     void *const alloc_pgtable_cb_info,
                     void *const free_pgtable_cb_info)
{
    const ptwalker_alloc_pgtable_t alloc_pgtable = walker->alloc_pgtable;
    assert(alloc_pgtable != NULL);

    pgt_level_t level = parent_level - 1;
    if (level == last_level) {
        if (!alloc_last_level) {
            walker->level = level + 1;
            return E_PT_WALKER_OK;
        }
    }

    do {
        // Get pte in (level + 1), where the index would be ((level + 1) - 1)
        pte_t *parent_table = walker->tables[level];
        pte_t *const pte_in_parent = &parent_table[walker->indices[level]];

        if (!alloc_table_at_pte(walker,
                                pte_in_parent,
                                level,
                                alloc_pgtable_cb_info))
        {
            walker->level = level + 1;
            ptwalker_drop_lowest(walker, free_pgtable_cb_info);

            return E_PT_WALKER_ALLOC_FAIL;
        }

        if (should_ref) {
            ref_up(&virt_to_page(parent_table)->table.refcount);
        }

        level--;
        if (level == last_level) {
            if (!alloc_last_level) {
                break;
            }
        } else if (level < last_level) {
            break;
        }
    } while (true);

    walker->level = level + 1;
    return E_PT_WALKER_OK;
}

__optimize(3) enum pt_walker_result
ptwalker_next_with_options(
    struct pt_walker *const walker,
    pgt_level_t level,
    const struct pt_walker_iterate_options *const options)
{
    // Bad increment as tables+indices haven't been filled down to the level
    // requested.

    if (__builtin_expect(walker->level > level || level > walker->top_level, 0))
    {
        return E_PT_WALKER_BAD_INCR;
    }

    pgt_index_t *indices_ptr = &walker->indices[level - 1];
    pgt_index_t index = ++*indices_ptr;

    if (index_in_bounds(index, PGT_PTE_COUNT(level))) {
        if (level == 1) {
            return E_PT_WALKER_OK;
        }

        pte_t *const pte = &walker->tables[level - 1][index];
        const pte_t entry = pte_read(pte);

        if (!pte_is_present(entry)) {
            reset_levels_lower_than(walker, level);
            return E_PT_WALKER_OK;
        }

        if (!pte_level_can_have_large(level)) {
            setup_levels_lower_than(walker, level, pte, entry);
            return E_PT_WALKER_OK;
        }

        if (pte_is_large(entry)) {
            reset_levels_lower_than(walker, level);
            walker->level = level;

            return E_PT_WALKER_OK;
        }

        setup_levels_lower_than(walker, level, pte, entry);
        return E_PT_WALKER_OK;
    }

    // Start tables from level + 1, as level is incremented soon anyways.
    pte_t **tables_ptr = &walker->tables[level];
    const pgt_level_t orig_level = level;

    do {
        level++;
        indices_ptr++;

        if (__builtin_expect(level > walker->top_level, 0)) {
            // Reset all of walker, except the root pointer
            *indices_ptr = 0;
            walker->level = PTWALKER_DONE;

            reset_levels_lower_than(walker, level);
            return E_PT_WALKER_REACHED_END;
        }

        index = ++*indices_ptr;
        if (index == PGT_PTE_COUNT(level)) {
            tables_ptr++;
            continue;
        }

        pte_t *const pte = &((*tables_ptr)[index]);
        const pte_t entry = pte_read(pte);

        if (!pte_is_present(entry)) {
            reset_levels_lower_than(walker, level);
            break;
        }

        if (!pte_level_can_have_large(level)) {
            setup_levels_lower_than(walker, level, pte, entry);
            return E_PT_WALKER_OK;
        }

        if (pte_is_large(entry)) {
            reset_levels_lower_than(walker, level);
            walker->level = level;

            return E_PT_WALKER_OK;
        }

        setup_levels_lower_than(walker, level, pte, entry);
        return E_PT_WALKER_OK;
    } while (true);

    if (!options->alloc_parents) {
        walker->level = level;
        return E_PT_WALKER_OK;
    }

    return alloc_levels_down_to(walker,
                                /*parent_level=*/level,
                                /*last_level=*/orig_level,
                                level,
                                options->should_ref,
                                options->alloc_pgtable_cb_info,
                                options->free_pgtable_cb_info);
}

__optimize(3)
enum pt_walker_result ptwalker_prev(struct pt_walker *const walker) {
    return ptwalker_prev_with_options(walker, walker->level, &default_options);
}

enum pt_walker_result
ptwalker_prev_with_options(
    struct pt_walker *const walker,
    pgt_level_t level,
    const struct pt_walker_iterate_options *const options)
{
    // Bad increment as tables+indices haven't been filled down to the level
    // requested.

    if (__builtin_expect(walker->level > level || level > walker->top_level, 0))
    {
        return E_PT_WALKER_BAD_INCR;
    }

    if (walker->level < level) {
        reset_levels_lower_than(walker, level);
    }

    pgt_index_t *indices_ptr = &walker->indices[level - 1];
    pgt_index_t index = *indices_ptr;

    if (index != 0) {
        index--;
        *indices_ptr = index;

        if (level == 1) {
            return E_PT_WALKER_OK;
        }

        pte_t *const pte = &walker->tables[level - 1][index];
        const pte_t entry = pte_read(pte);

        if (!pte_is_present(entry)) {
            reset_levels_lower_than(walker, level);
            return E_PT_WALKER_OK;
        }

        if (!pte_level_can_have_large(level)) {
            setup_levels_lower_than(walker, level, pte, entry);
            return E_PT_WALKER_OK;
        }

        if (pte_is_large(entry)) {
            reset_levels_lower_than(walker, level);
            walker->level = level;

            return E_PT_WALKER_OK;
        }

        setup_levels_lower_than(walker, level, pte, entry);
        return E_PT_WALKER_OK;
    }

    pte_t **tables_ptr = &walker->tables[level];
    *indices_ptr = PGT_PTE_COUNT(level) - 1;

    const pgt_level_t orig_level = level;

    do {
        level++;
        indices_ptr++;

        if (__builtin_expect(level > walker->top_level, 0)) {
            // Reset all of walker, except the root pointer
            *indices_ptr = 0;
            walker->level = PTWALKER_DONE;

            reset_levels_lower_than(walker, level);
            return E_PT_WALKER_REACHED_END;
        }

        index = *indices_ptr;
        if (index == 0) {
            *indices_ptr = PGT_PTE_COUNT(level) - 1;
            tables_ptr++;

            continue;
        }

        index--;
        *indices_ptr = index;

        pte_t *const pte = &((*tables_ptr)[index]);
        const pte_t entry = pte_read(pte);

        if (!pte_level_can_have_large(level)) {
            setup_levels_lower_than(walker, level, pte, entry);
            return E_PT_WALKER_OK;
        }

        if (!pte_is_present(entry)) {
            reset_levels_lower_than(walker, level);
            break;
        }

        if (pte_is_large(entry)) {
            reset_levels_lower_than(walker, level);
            walker->level = level;

            return E_PT_WALKER_OK;
        }

        setup_levels_lower_than(walker, level, pte, entry);
        return E_PT_WALKER_OK;
    } while (true);

    if (!options->alloc_parents) {
        walker->level = level;
        return E_PT_WALKER_OK;
    }

    return alloc_levels_down_to(walker,
                                /*parent_level=*/level,
                                /*last_level=*/orig_level,
                                level,
                                options->should_ref,
                                options->alloc_pgtable_cb_info,
                                options->free_pgtable_cb_info);
}

enum pt_walker_result
ptwalker_fill_in_to(struct pt_walker *const walker,
                    const pgt_level_t level,
                    const bool should_ref,
                    void *const alloc_pgtable_cb_info,
                    void *const free_pgtable_cb_info)
{
    if (walker->level <= level) {
        return E_PT_WALKER_OK;
    }

    return alloc_levels_down_to(walker,
                                walker->level,
                                level,
                                /*alloc_level=*/true,
                                should_ref,
                                alloc_pgtable_cb_info,
                                free_pgtable_cb_info);
}

__optimize(3) void
ptwalker_deref_from_level(struct pt_walker *const walker,
                          pgt_level_t level,
                          void *const free_pgtable_cb_info)
{
    if (__builtin_expect(level < walker->level || level > walker->top_level, 0))
    {
        return;
    }

    const ptwalker_free_pgtable_t free_pgtable = walker->free_pgtable;
    assert(free_pgtable != NULL);

    pte_t *table = walker->tables[level - 1];
    for (; level <= walker->top_level; level++) {
        struct page *const pt = virt_to_page(table);
        if (!ref_down(&pt->table.refcount)) {
            break;
        }

        if (__builtin_expect(level != walker->top_level, 1)) {
            table = walker->tables[level];
            pte_write(&table[walker->indices[level]], /*value=*/0);
        }

        free_pgtable(walker, pt, free_pgtable_cb_info);
    }

    walker->level = level;
}

// FIXME: This doesn't work and is also arch-dependent
uint64_t ptwalker_get_virt_addr(const struct pt_walker *const walker) {
    uint64_t result = 0;
    for (pgt_level_t level = walker->level; level <= walker->top_level; level++)
    {
        const uint64_t index = (uint64_t)walker->indices[level - 1];
        result |= (index & PT_LEVEL_MASKS[level]) << PAGE_SHIFTS[level - 1];
    }

    return result;
}

uint64_t
ptwalker_virt_get_phys(struct pagemap *const pagemap, const uint64_t virt) {
    struct pt_walker walker;
    ptwalker_default_for_pagemap(&walker, pagemap, virt);

    if (__builtin_expect(
            walker.level > 1 && !pte_level_can_have_large(walker.level), 0))
    {
        return INVALID_PHYS;
    }

    const pte_t pte =
        pte_read(walker.tables[walker.level - 1] +
                 walker.indices[walker.level - 1]);

    if (!pte_is_present(pte)) {
        return INVALID_PHYS;
    }

    if (walker.level != 1 && !pte_is_large(pte)) {
        return INVALID_PHYS;
    }

    const uint64_t offset =
        virt & mask_for_n_bits(PAGE_SHIFTS[walker.level - 1]);

    return pte_to_phys(pte) + offset;
}

bool ptwalker_points_to_largepage(const struct pt_walker *const walker) {
    if (walker->level <= 1 || !pte_level_can_have_large(walker->level)) {
        return false;
    }

    const pte_t pte =
        pte_read(walker->tables[walker->level - 1] +
                 walker->indices[walker->level - 1]);

    return pte_is_large(pte);
}