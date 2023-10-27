/*
 * lib/overflow.h
 * © suhas pai
 */

#pragma once
#include "lib/assert.h"

#define check_add(lhs, rhs, result) (!__builtin_add_overflow(lhs, rhs, result))
#define check_sub(lhs, rhs, result) (!__builtin_sub_overflow(lhs, rhs, result))
#define check_mul(lhs, rhs, result) (!__builtin_mul_overflow(lhs, rhs, result))

#define check_add_assert(lhs, rhs) ({ \
    __auto_type __result__ = lhs; \
    assert(__builtin_expect(check_add(lhs, rhs, &__result__), 1)); \
    __result__; \
})

#define check_sub_assert(lhs, rhs) ({ \
    __auto_type __result__ = lhs; \
    assert(__builtin_expect(check_sub(lhs, rhs, &__result__), 1)); \
    __result__; \
})

#define check_mul_assert(lhs, rhs) ({ \
    __auto_type __result__ = lhs; \
    assert(__builtin_expect(check_mul(lhs, rhs, &__result__), 1)); \
    __result__; \
})
