/*
 * lib/adt/mutable_buffer.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "string_view.h"

struct mutable_buffer {
    void *begin;
    const void *end;

    uint32_t index;
};

#define MBUFFER_STATIC_STACK(name, size) \
    char RAND_VAR_NAME()[size];          \
    bzero(RAND_VAR_NAME(), (size));      \
    struct mutable_buffer name =         \
        mbuffer_open_static(RAND_VAR_NAME(), /*used=*/0, (size))

struct mutable_buffer
mbuffer_open(void *buffer, uint32_t used, uint32_t capacity);

struct mutable_buffer
mbuffer_open_static(void *buffer, uint32_t used, uint32_t capacity);

void *mbuffer_current_ptr(struct mutable_buffer mbuffer);

uint32_t mbuffer_free_space(struct mutable_buffer mbuffer);
uint32_t mbuffer_used_size(struct mutable_buffer mbuffer);
uint32_t mbuffer_capacity(struct mutable_buffer mbuffer);

bool mbuffer_can_add_size(struct mutable_buffer mbuffer, uint32_t size);

bool mbuffer_empty(struct mutable_buffer mbuffer);
bool mbuffer_full(struct mutable_buffer mbuffer);

uint32_t mbuffer_incr_ptr(struct mutable_buffer *mbuffer, uint32_t amt);
uint32_t mbuffer_decr_ptr(struct mutable_buffer *mbuffer, uint32_t amt);

uint32_t
mbuffer_append_data(struct mutable_buffer *mbuffer,
                    const void *data,
                    uint32_t length);

uint32_t
mbuffer_append_byte(struct mutable_buffer *mbuffer,
                    const uint8_t byte,
                    uint32_t count);

uint32_t
mbuffer_append_sv(struct mutable_buffer *mbuffer, struct string_view sv);

bool mbuffer_truncate(struct mutable_buffer *mbuffer, uint32_t used_size);