/*
 * kernel/src/mm/zone.c
 * © suhas pai
 */

#include "zone.h"

__debug_optimize(3)
struct page_zone *page_to_zone(const struct page *const page) {
    return page_to_section(page)->zone;
}