/*
 * kernel/include/mm/simple_alloc.h
 * © suhas pai
 */

#pragma once
#include <lib/list.h>

#define SIMPLE_ALLOC_MAX 512

// Simple allocator for small allocations, not thread-safe by default.
// Uses a linked list of free pages, with each page having a bump allocator.
//
// Note that for this reason allocations cannot be larger than PAGE_SIZE.
//
// Objects are never freed, instead each page has a refcount and the page is
// freed when the refcount hits 0.

struct simple_alloc {
    struct list page_list;
};

#define SIMPLE_ALLOC_INIT(name) \
    ((struct simple_alloc){ .page_list = LIST_INIT(&name.page_list) })

bool simple_alloc_initialized(const struct simple_alloc *const alloc);

void simple_alloc_init(struct simple_alloc *alloc);
void simple_alloc_destroy(struct simple_alloc *alloc);

void *simple_alloc(struct simple_alloc *alloc, uint32_t size);
void simple_free(struct simple_alloc *alloc, void *buffer);

bool simple_try_free(struct simple_alloc *alloc, void *buffer);

void *
simple_alloc_size(struct simple_alloc *alloc,
                  uint32_t size,
                  uint32_t *size_out);
