/*
 * kernel/mm/zone.c
 * © suhas pai
 */

#include "zone.h"

__optimize(3) struct page_zone *page_to_zone(const struct page *const page) {
    return phys_to_zone(page_to_phys(page));
}