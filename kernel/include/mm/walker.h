/*
 * kernel/src/mm/walker.h
 * © suhas pai
 */

#pragma once
#include "mm_types.h"

#define PTWALKER_CLEAR 0
#define PTWALKER_DONE -1

struct pg_walker;

typedef uint64_t
(*pgwalker_alloc_pgtable_t)(struct pg_walker *walker,
                            pg_level_t level,
                            void *cb_info);

typedef void
(*pgwalker_free_pgtable_t)(struct pg_walker *walker,
                           struct page *pt,
                           void *cb_info);

struct pg_walker {
    // tables and indices are stored from top to bottom.
    // i.e. tables[0] is the top level

    pte_t *tables[PGT_LEVEL_COUNT];
    pg_index_t indices[PGT_LEVEL_COUNT];

    // level should always start from 1..=PGT_LEVEL_COUNT.
    int16_t level;
    pg_level_t top_level;

    pgwalker_alloc_pgtable_t alloc_pgtable;
    pgwalker_free_pgtable_t free_pgtable;
};

#define PGWALKER_INIT() \
    ((struct pg_walker){ \
        .tables = {0}, \
        .indices = {0}, \
        .level = 0, \
        .top_level = 0, \
        .alloc_pgtable = NULL, \
        .free_pgtable = NULL \
    })

uint64_t
pgwalker_early_alloc_pgtable_cb(struct pg_walker *walker,
                                pg_level_t level,
                                void *cb_info);

void pgwalker_default(struct pg_walker *walker, uint64_t virt_addr);

struct pagemap;
void
pgwalker_default_for_pagemap(struct pg_walker *walker,
                             const struct pagemap *pagemap,
                             uint64_t virt_addr);

void
pgwalker_create(struct pg_walker *walker,
                uint64_t virt_addr,
                pgwalker_alloc_pgtable_t alloc_pgtable,
                pgwalker_free_pgtable_t free_pgtable);

void
pgwalker_create_for_pagemap(struct pg_walker *walker,
                            const struct pagemap *pagemap,
                            uint64_t virt_addr,
                            pgwalker_alloc_pgtable_t alloc_pgtable,
                            pgwalker_free_pgtable_t free_pgtable);

void
pgwalker_create_from_root_phys(struct pg_walker *walker,
                               uint64_t root_phys,
                               uint64_t virt_addr,
                               pgwalker_alloc_pgtable_t alloc_pgtable,
                               pgwalker_free_pgtable_t free_pgtable);

void
pgwalker_create_from_toplevel(struct pg_walker *walker,
                              uint64_t root_phys,
                              pg_level_t top_level,
                              pg_index_t root_index,
                              pgwalker_alloc_pgtable_t alloc_pgtable,
                              pgwalker_free_pgtable_t free_pgtable);

enum pgwalker_result {
    E_PGWALKER_OK,
    E_PGWALKER_REACHED_END,
    E_PGWALKER_ALLOC_FAIL,
    E_PGWALKER_BAD_INCR
};

enum pgwalker_result pgwalker_prev(struct pg_walker *walker);
enum pgwalker_result pgwalker_next(struct pg_walker *walker);

struct pgwalker_iterate_options {
    void *alloc_pgtable_cb_info;
    void *free_pgtable_cb_info;

    bool alloc_parents : 1;
    bool alloc_level : 1;
    bool should_ref : 1;
};

enum pgwalker_result
pgwalker_prev_with_options(struct pg_walker *walker,
                           pg_level_t level,
                           const struct pgwalker_iterate_options *options);

enum pgwalker_result
pgwalker_next_with_options(struct pg_walker *walker,
                           pg_level_t level,
                           const struct pgwalker_iterate_options *options);

void
pgwalker_deref_from_level(struct pg_walker *walker,
                          pg_level_t level,
                          void *free_pgtable_cb_info);

enum pgwalker_result
pgwalker_fill_in_to(struct pg_walker *walker,
                    pg_level_t level,
                    bool should_ref,
                    void *alloc_pgtable_cb_info,
                    void *free_pgtable_cb_info);

uint64_t pgwalker_get_virt_addr(const struct pg_walker *walker);
uint64_t pgwalker_get_phys_addr(const struct pg_walker *walker);

bool pgwalker_points_to_largepage(const struct pg_walker *walker);
