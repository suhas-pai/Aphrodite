/*
 * kernel/mm/pageop.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/range.h"
#include "lib/list.h"

#include "mm_types.h"

struct pagemap;
struct pageop {
    struct pagemap *pagemap;
    struct range flush_range;
    struct list delayed_free;
};

#define PAGEOP_INIT(name) \
    ((struct pageop){ \
        .flush_range = RANGE_EMPTY(), \
        .delayed_free = LIST_INIT(name.delayed_free) \
    })

void
pageop_init(struct pageop *pageop,
            struct pagemap *pagemap,
            struct range range);

void
pageop_flush_pte_in_current_range(struct pageop *pageop,
                                  const pte_t pte,
                                  pgt_level_t level,
                                  const bool should_free_pages);

void pageop_setup_for_address(struct pageop *pageop, uint64_t virt);
void pageop_setup_for_range(struct pageop *pageop, struct range virt);

void pageop_finish(struct pageop *pageop);