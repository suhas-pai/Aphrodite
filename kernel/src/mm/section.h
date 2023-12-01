/*
 * kernel/src/mm/section.h
 * © suhas pai
 */

#pragma once
#include "lib/adt/range.h"

#include "cpu/spinlock.h"
#include "lib/list.h"

#include "mm/types.h"

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

    // max_order in section is one plus the highest order that has at least one
    // page in its freelist.

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
struct page_section *phys_to_section(uint64_t phys);
struct page_section *pfn_to_section(uint64_t phys);

