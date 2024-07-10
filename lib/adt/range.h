/*
 * lib/adt/range.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lib/inttypes.h"

struct range {
    uint64_t front;
    uint64_t size;
};

#define RANGE_FMT "0x%" PRIx64 " - 0x%" PRIx64
#define RANGE_FMT_ARGS(range) \
    (range).front,            \
    ({                        \
        uint64_t __end__ = 0; \
        range_get_end((range), &__end__); \
        __end__; \
    })

#define RANGE_EMPTY() ((struct range){ .front = 0, .size = 0 })
#define RANGE_MAX() ((struct range){ .front = 0, .size = UINT64_MAX })
#define RANGE_INIT(front_, size_) \
    ((struct range){ .front = (front_), .size = (size_) })

#define rangeof_field(type, field) \
    RANGE_INIT(offsetof(type, field), sizeof_field(type, field))

#define range_iterate(range, iter, incr) \
    for (uint64_t iter = range.front; \
         (iter - range.front) < range.size; \
         iter += (incr))

bool range_create_and_verify(uint64_t base, uint64_t size, struct range *out);

struct range range_create_upto(uint64_t size);
struct range range_create_end(uint64_t front, uint64_t size);

struct range range_from_index(struct range range, uint64_t index);
struct range range_from_loc(struct range range, uint64_t loc);

struct range range_divide(struct range range, uint64_t div);
struct range range_divide_out(struct range range, uint64_t div);

bool range_multiply(struct range range, uint64_t mult, struct range *out);
bool range_round_up(struct range range, uint64_t mult, struct range *out);

bool
range_round_up_subrange(struct range range, uint64_t mult, struct range *out);

bool range_align_in(struct range range, uint64_t boundary, struct range *out);
bool range_align_out(struct range range, uint64_t boundary, struct range *out);

bool range_has_index(struct range range, uint64_t index);
bool range_has_loc(struct range range, uint64_t loc);
bool range_has_end(struct range range, uint64_t loc);
bool range_get_end(struct range range, uint64_t *end_out);

bool range_overflows(struct range range);

bool range_above(struct range range, struct range above);
bool range_below(struct range range, struct range below);
bool range_empty(struct range range);

bool range_is_loc_above(struct range range, uint64_t index);
bool range_is_loc_below(struct range range, uint64_t index);

bool range_has_align(struct range range, uint64_t align);
uint64_t range_get_end_assert(struct range range);

struct range subrange_from_index(struct range range, uint64_t index);
struct range subrange_to_full(struct range range, struct range index);

uint64_t range_loc_for_index(struct range range, uint64_t index);
uint64_t range_index_for_loc(struct range range, uint64_t loc);

bool range_has(struct range range, struct range other);
bool range_has_index_range(struct range range, struct range other);
bool range_overlaps(struct range range, struct range other);
