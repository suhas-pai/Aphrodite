/*
 * lib/refcount.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "assert.h"
#include "refcount.h"

__optimize(3) void refcount_init(struct refcount *const ref) {
    ref->count = 1;
}

__optimize(3)
void refcount_increment(struct refcount *const ref, const int32_t amount) {
    const int32_t old =
        atomic_fetch_add_explicit(&ref->count, amount, memory_order_relaxed);

    assert_msg(old >= 0, "UAF in refcount_increment()");
    assert_msg(old != REFCOUNT_MAX,
               "refcount_increment() called on maxed refcount");
}

__optimize(3)
bool refcount_decrement(struct refcount *const ref, const int32_t amount) {
    const int32_t old =
        atomic_fetch_sub_explicit(&ref->count, amount, memory_order_relaxed);

    assert_msg(old >= amount, "UAF in refcount_decrement()");
    assert_msg(old != REFCOUNT_MAX,
               "refcount_decrement() called on maxed refcount");

    return old == 1;
}

__optimize(3) void ref_up(struct refcount *const ref) {
    refcount_increment(ref, /*amount=*/1);
}

__optimize(3) bool ref_down(struct refcount *const ref) {
    return refcount_decrement(ref, /*amount=*/1);
}

__optimize(3) uint32_t ref_get(struct refcount *const ref) {
    return (uint32_t)atomic_load_explicit(&ref->count, memory_order_relaxed);
}