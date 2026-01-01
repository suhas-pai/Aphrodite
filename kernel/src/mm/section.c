/*
 * kernel/src/mm/section.c
 * © suhas pai
 */

#include <lib/overflow.h>

#include "mm/section.h"
#include "mm/page.h"
#include "mm/zone.h"

__debug_optimize(3) void
page_section_init(struct page_section *const section,
                  struct page_zone *const zone,
                  const struct range range,
                  const uint64_t pfn)
{
    section->zone = zone;
    section->lock = SPINLOCK_INIT();
    section->pfn = pfn;
    section->range = range;
    section->min_order = MAX_ORDER;
    section->max_order = 0;
    section->total_free = 0;

    carr_foreach(section->freelist_list, freelist) {
        list_init(&freelist->page_list);
        freelist->count = 0;
    }

    list_init(&section->zone_list);
}

__debug_optimize(3) struct range
page_section_get_pfn_range(const struct page_section *const section) {
    return RANGE_INIT(section->pfn, PAGE_COUNT(section->range.size));
}

__debug_optimize(3) struct page_section *phys_to_section(const uint64_t phys) {
    mm_for_each_page_section(iter) {
        if (range_has_loc(iter->range, phys)) {
            return iter;
        }
    }

    verify_not_reached();
}

__debug_optimize(3) struct page_section *pfn_to_section(const uint64_t pfn) {
    mm_for_each_page_section(iter) {
        const auto pfn_range =
            RANGE_INIT(iter->pfn, PAGE_COUNT(iter->range.size));

        if (range_has_loc(pfn_range, pfn)) {
            return iter;
        }
    }

    verify_not_reached();
}

__debug_optimize(3) uint64_t phys_to_pfn(const uint64_t phys) {
    mm_for_each_page_section(iter) {
        if (range_has_loc(iter->range, phys)) {
            const uint64_t relative_phys =
                range_index_for_loc(iter->range, phys);

            return iter->pfn + PAGE_COUNT(relative_phys);
        }
    }

    verify_not_reached();
}

__debug_optimize(3) uint64_t page_to_phys(const struct page *const page) {
    assert((uint64_t)page >= PAGE_OFFSET && (uint64_t)page < PAGE_END);
    const auto section = page_to_section(page);

    const uint64_t page_pfn = page_to_pfn(page);
    const uint64_t relative_pfn = ckd_sub_assert(page_pfn, section->pfn);

    return section->range.front + (relative_pfn << PAGE_SHIFT);
}

__debug_optimize(3) uint64_t pfn_to_phys_manual(const uint64_t pfn) {
    mm_for_each_page_section(iter) {
        const auto pfn_range =
            RANGE_INIT(iter->pfn, PAGE_COUNT(iter->range.size));

        if (range_has_loc(pfn_range, pfn)) {
            const uint64_t index = range_index_for_loc(pfn_range, pfn);
            return iter->range.front + (index << PAGE_SHIFT);
        }
    }

    verify_not_reached();
}
