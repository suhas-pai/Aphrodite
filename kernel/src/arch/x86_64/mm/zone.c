/*
 * kernel/arch/x86_64/mm/zone.c
 * © suhas pai
 */

#include "lib/size.h"
#include "mm/zone.h"

static struct page_zone zone_low4g = {
    .lock = SPINLOCK_INIT(),
    .name = "low4g",

    .section_list = LIST_INIT(zone_low4g.section_list),
    .fallback_zone = NULL,
};

static struct page_zone zone_default = {
    .lock = SPINLOCK_INIT(),
    .name = "default",

    .section_list = LIST_INIT(zone_default.section_list),
    .fallback_zone = &zone_low4g,
};

__optimize(3) struct page_zone *phys_to_zone(const uint64_t phys) {
    if (phys < gib(4)) {
        return &zone_low4g;
    }

    return &zone_default;
}

__optimize(3) struct page_zone *page_zone_iterstart() {
    return &zone_default;
}

__optimize(3)
struct page_zone *page_zone_iternext(struct page_zone *const zone) {
    return zone->fallback_zone;
}

__optimize(3) struct page_zone *page_zone_default() {
    return &zone_default;
}

__optimize(3) struct page_zone *page_zone_low4g() {
    return &zone_low4g;
}

void pagezones_init() {}