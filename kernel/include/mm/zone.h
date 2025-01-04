/*
 * kernel/src/mm/zone.h
 * © suhas pai
 */

#pragma once
#include "page_alloc.h"

struct page_zone {
    struct spinlock lock;
    const char *const name;

    struct list section_list;
    struct page_zone *const fallback_zone;

    _Atomic uint64_t total_free;
};

struct page_zone *page_zoneiter_start();
struct page_zone *page_zoneiter_next(struct page_zone *prev);

struct page_zone *page_to_zone(const struct page *page);
struct page_zone *phys_to_zone(uint64_t phys);

struct page_zone *page_zone_default();
struct page_zone *page_zone_low4g();

#define for_each_page_zone(zone) \
    for (__auto_type zone = page_zoneiter_start(); \
         zone != nullptr;                             \
         zone = page_zoneiter_next(zone))
