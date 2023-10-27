/*
 * lib/adt/growable_buffer.h
 * © suhas pai
 */

#pragma once

#include "mutable_buffer.h"
#include "range.h"

struct growable_buffer {
    void *begin;
    const void *end;

    uint32_t index;
    bool is_alloc : 1;
};

#define GBUFFER_INIT() \
    ((struct growable_buffer){ \
        .begin = NULL,         \
        .index = 0,            \
        .end = NULL,           \
        .is_alloc = false      \
    })

#define GBUFFER_FROM_PTR(ptr, capacity) \
    ((struct growable_buffer){ \
        .begin = (ptr),        \
        .index = 0,            \
        .end = (ptr) + (capacity), \
        .is_alloc = false \
    })

struct growable_buffer gbuffer_alloc(uint32_t capacity);
struct growable_buffer gbuffer_alloc_copy(void *data, const uint32_t size);

struct growable_buffer
gbuffer_open(void *buffer, uint32_t used, uint32_t capacity, bool is_alloc);

struct growable_buffer
gbuffer_open_mbuffer(struct mutable_buffer mbuffer, bool is_alloc);

bool gbuffer_ensure_can_add_capacity(struct growable_buffer *gb, uint32_t add);

struct mutable_buffer
gbuffer_get_mutable_buffer(struct growable_buffer gbuffer);

void *gbuffer_current_ptr(struct growable_buffer gbuffer);
void *gbuffer_at(struct growable_buffer gbuffer, uint32_t index);

uint32_t gbuffer_free_space(struct growable_buffer gbuffer);
uint32_t gbuffer_used_size(struct growable_buffer gbuffer);
uint32_t gbuffer_capacity(struct growable_buffer gbuffer);

bool gbuffer_can_add_size(struct growable_buffer gbuffer, uint32_t size);
bool gbuffer_empty(struct growable_buffer gbuffer);

uint32_t gbuffer_incr_ptr(struct growable_buffer *gbuffer, uint32_t amt);
uint32_t gbuffer_decr_ptr(struct growable_buffer *gbuffer, uint32_t amt);

uint32_t
gbuffer_append_data(struct growable_buffer *gbuffer,
                    const void *data,
                    uint32_t length);

uint32_t
gbuffer_append_byte(struct growable_buffer *gbuffer,
                    uint8_t byte,
                    uint32_t count);

bool
gbuffer_append_gbuffer_data(struct growable_buffer *gbuffer,
                            const struct growable_buffer *append);

uint32_t
gbuffer_append_sv(struct growable_buffer *gbuffer, struct string_view sv);

void gbuffer_remove_index(struct growable_buffer *gbuffer, uint32_t index);
void gbuffer_remove_range(struct growable_buffer *gbuffer, struct range range);

void gbuffer_truncate(struct growable_buffer *gbuffer, uint32_t byte_index);
void *gbuffer_take_data(struct growable_buffer *gbuffer);

void gbuffer_clear(struct growable_buffer *gbuffer);
void gbuffer_destroy(struct growable_buffer *gbuffer);