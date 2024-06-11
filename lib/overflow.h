/*
 * lib/overflow.h
 * © suhas pai
 */

#pragma once
#include "lib/assert.h"

#define check_add(lhs, rhs, result) (!__builtin_add_overflow(lhs, rhs, result))
#define check_sub(lhs, rhs, result) (!__builtin_sub_overflow(lhs, rhs, result))
#define check_mul(lhs, rhs, result) (!__builtin_mul_overflow(lhs, rhs, result))

#define check_ptr_add(lhs, rhs, result) \
    (!__builtin_add_overflow((uint64_t)lhs, rhs, (uint64_t *)result))

#define check_ptr_sub(lhs, rhs, result) \
    (!__builtin_sub_overflow((uint64_t)lhs, rhs, (uint64_t *)result))

#define check_add_assert(lhs, rhs) ({ \
    __auto_type __checkaddassertresult__ = lhs; \
    assert(check_add(lhs, rhs, &__checkaddassertresult__)); \
    __checkaddassertresult__; \
})

#define check_sub_assert(lhs, rhs) ({ \
    __auto_type __checksubassertresult__ = lhs; \
    assert(check_sub(lhs, rhs, &__checksubassertresult__)); \
    __checksubassertresult__; \
})

#define check_mul_assert(lhs, rhs) ({ \
    __auto_type __checkmulassertresult__ = lhs; \
    assert(check_mul(lhs, rhs, &__checkmulassertresult__)); \
    __checkmulassertresult__; \
})
