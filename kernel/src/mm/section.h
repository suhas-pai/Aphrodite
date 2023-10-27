/*
 * kernel/mm/section.h
 * © suhas pai
 */

#pragma once
#include "cpu/spinlock.h"

#include "lib/adt/range.h"
#include "lib/list.h"

#include "mm_types.h"

struct page_freelist {
    struct list page_list;
    uint64_t count;
};

struct page_zone;
struct page_section {
    struct page_zone *zone;

    struct list zone_list;
    struct range range;

    uint64_t pfn;

    // Lock covers all fields directly after it.
    struct spinlock lock;
    struct page_freelist freelist_list[MAX_ORDER];

    uint8_t min_order;
    uint8_t max_order;

    uint64_t total_free;
};

void
page_section_init(struct page_section *section,
                  struct page_zone *zone,
                  struct range range,
                  uint64_t pfn);

struct page_section *mm_get_page_section_list();
