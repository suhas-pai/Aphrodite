/*
 * kernel/include/mm/simple_alloc.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/array.h"
#include "lib/macros.h"

#define SIMPLE_ALLOC_MAX 512

// Simple allocator for small allocations, not thread-safe by default.
// Uses a linked list of free pages, with each page having a bump allocator.
//
// Objects are never freed, instead each page has a refcount and the page is
// freed when the refcount hits 0.

struct simple_alloc {
    struct array page_list;
};

void simple_alloc_create(struct simple_alloc *alloc);
void simple_alloc_destroy(struct simple_alloc *alloc);

void *simple_alloc(struct simple_alloc *alloc, uint32_t size);
void simple_free(struct simple_alloc *alloc, void *buffer);

bool simple_try_free(struct simple_alloc *alloc, void *buffer);

void *
simple_alloc_size(struct simple_alloc *alloc,
                  uint32_t size,
                  uint32_t *size_out);
