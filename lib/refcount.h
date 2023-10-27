/*
 * lib/refcount.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

struct refcount {
    _Atomic int32_t count;
};

#define REFCOUNT_SATURATED (INT32_MAX / 2)
#define REFCOUNT_MAX INT32_MAX

#define REFCOUNT_EMPTY() ((struct refcount){ .count = 0 })
#define REFCOUNT_CREATE(amt) ((struct refcount){ .count = (amt) })
#define REFCOUNT_CREATE_MAX() ((struct refcount){ .count = REFCOUNT_MAX })

void refcount_init(struct refcount *ref);

void refcount_increment(struct refcount *ref, int32_t amount);
bool refcount_decrement(struct refcount *ref, int32_t amount);

void ref_up(struct refcount *ref);
bool ref_down(struct refcount *ref);

