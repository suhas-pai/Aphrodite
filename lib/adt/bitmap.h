/*
 * lib/adt/bitmap.h
 * © suhas pai
 */

#pragma once

#include "lib/bits.h"
#include "growable_buffer.h"

struct bitmap {
    struct growable_buffer gbuffer;
};

#define BITMAP_INIT() ((struct bitmap){ .gbuffer = GBUFFER_INIT() })
#define BITMAP_PTR(ptr, capacity) \
    ((struct bitmap){ .gbuffer = GBUFFER_FROM_PTR(ptr, capacity) })

struct bitmap bitmap_alloc(uint64_t bit_count);
struct bitmap bitmap_open(void *buffer, uint64_t byte_count);

uint64_t bitmap_capacity(const struct bitmap *bitmap);

uint64_t
bitmap_find(struct bitmap *bitmap,
            uint64_t count,
            uint64_t start_index,
            bool expected_value,
            bool invert);

uint64_t
bitmap_find_at_mult(struct bitmap *bitmap,
                    uint64_t count,
                    uint64_t mult,
                    uint64_t start_index,
                    bool expected_value,
                    bool invert);

bool bitmap_at(const struct bitmap *bitmap, uint64_t index);
bool bitmap_has(const struct bitmap *bitmap, struct range range, bool value);

void bitmap_set(struct bitmap *bitmap, uint64_t index, bool value);
void bitmap_set_range(struct bitmap *bitmap, struct range range, bool value);

void bitmap_set_all(struct bitmap *bitmap, bool value);
void bitmap_destroy(struct bitmap *bitmap);