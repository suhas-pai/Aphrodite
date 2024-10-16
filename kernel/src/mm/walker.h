/*
 * kernel/src/mm/walker.h
 * © suhas pai
 */

#pragma once
#include "mm_types.h"

#define PTWALKER_CLEAR 0
#define PTWALKER_DONE -1

struct pt_walker;

typedef uint64_t
(*ptwalker_alloc_pgtable_t)(struct pt_walker *walker,
                            pgt_level_t level,
                            void *cb_info);

typedef void
(*ptwalker_free_pgtable_t)(struct pt_walker *walker,
                           struct page *pt,
                           void *cb_info);

struct pt_walker {
    // tables and indices are stored from top to bottom.
    // i.e. tables[0] is the top level

    pte_t *tables[PGT_LEVEL_COUNT];
    pgt_index_t indices[PGT_LEVEL_COUNT];

    // level should always start from 1..=PGT_LEVEL_COUNT.
    int16_t level;
    pgt_level_t top_level;

    ptwalker_alloc_pgtable_t alloc_pgtable;
    ptwalker_free_pgtable_t free_pgtable;
};

#define PT_WALKER_INIT() \
    ((struct pt_walker){ \
        .tables = {0},   \
        .indices = {0},  \
        .level = 0,      \
        .top_level = 0,  \
        .alloc_pgtable = NULL, \
        .free_pgtable = NULL   \
    })

uint64_t
ptwalker_early_alloc_pgtable_cb(struct pt_walker *walker,
                                pgt_level_t level,
                                void *cb_info);

void ptwalker_default(struct pt_walker *walker, uint64_t virt_addr);

struct pagemap;
void
ptwalker_default_for_pagemap(struct pt_walker *walker,
                             const struct pagemap *pagemap,
                             uint64_t virt_addr);

void
ptwalker_create(struct pt_walker *walker,
                uint64_t virt_addr,
                ptwalker_alloc_pgtable_t alloc_pgtable,
                ptwalker_free_pgtable_t free_pgtable);

void
ptwalker_create_for_pagemap(struct pt_walker *walker,
                            const struct pagemap *pagemap,
                            uint64_t virt_addr,
                            ptwalker_alloc_pgtable_t alloc_pgtable,
                            ptwalker_free_pgtable_t free_pgtable);

void
ptwalker_create_from_root_phys(struct pt_walker *walker,
                               uint64_t root_phys,
                               uint64_t virt_addr,
                               ptwalker_alloc_pgtable_t alloc_pgtable,
                               ptwalker_free_pgtable_t free_pgtable);

void
ptwalker_create_from_toplevel(struct pt_walker *walker,
                              uint64_t root_phys,
                              pgt_level_t top_level,
                              pgt_index_t root_index,
                              ptwalker_alloc_pgtable_t alloc_pgtable,
                              ptwalker_free_pgtable_t free_pgtable);

enum pt_walker_result {
    E_PT_WALKER_OK,
    E_PT_WALKER_REACHED_END,
    E_PT_WALKER_ALLOC_FAIL,
    E_PT_WALKER_BAD_INCR
};

enum pt_walker_result ptwalker_prev(struct pt_walker *walker);
enum pt_walker_result ptwalker_next(struct pt_walker *walker);

struct ptwalker_iterate_options {
    void *alloc_pgtable_cb_info;
    void *free_pgtable_cb_info;

    bool alloc_parents : 1;
    bool alloc_level : 1;
    bool should_ref : 1;
};

enum pt_walker_result
ptwalker_prev_with_options(struct pt_walker *walker,
                           pgt_level_t level,
                           const struct ptwalker_iterate_options *options);

enum pt_walker_result
ptwalker_next_with_options(struct pt_walker *walker,
                           pgt_level_t level,
                           const struct ptwalker_iterate_options *options);

void
ptwalker_deref_from_level(struct pt_walker *walker,
                          pgt_level_t level,
                          void *free_pgtable_cb_info);

enum pt_walker_result
ptwalker_fill_in_to(struct pt_walker *walker,
                    pgt_level_t level,
                    bool should_ref,
                    void *alloc_pgtable_cb_info,
                    void *free_pgtable_cb_info);

uint64_t ptwalker_get_virt_addr(const struct pt_walker *walker);
uint64_t ptwalker_get_phys_addr(const struct pt_walker *walker);

bool ptwalker_points_to_largepage(const struct pt_walker *walker);
