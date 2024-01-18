/*
 * kernel/src/mm/page_alloc.h
 * © suhas pai
 */

#pragma once

#include "page.h"
#include "pageop.h"

#define alloc_page(state, flags) alloc_pages((state), (flags), /*order=*/0)
#define free_page(page) free_pages((page), /*order=*/0)

enum page_alloc_flags {
    __ALLOC_ZERO = 1 << 0,
};

// free_pages will call zero-out the page. Call page_to_zone() and
// free_pages_to_zone() directly to avoid this.

void free_pages(struct page *page, uint8_t order);
void free_large_page(struct page *page);

struct page *deref_page(struct page *page, struct pageop *pageop);

// We may not be necessarily derefing a large page, just a continuous set of
// pages mapped as a large page.

struct page *
deref_large_page(struct page *page, struct pageop *pageop, pgt_level_t level);

struct page *
alloc_pages(enum page_state state, uint64_t alloc_flags, uint8_t order);

struct page_zone;

struct page *
alloc_pages_from_zone(struct page_zone *zone,
                      enum page_state state,
                      uint64_t alloc_flags,
                      uint8_t order,
                      bool fallback);

struct page *alloc_large_page(pgt_level_t level, uint64_t flags);
struct page *
alloc_large_page_in_zone(struct page_zone *zone,
                         uint64_t alloc_flags,
                         pgt_level_t level,
                         bool allow_fallback);

struct page *alloc_table();
struct page *alloc_user_stack(struct process *proc, uint8_t order);

