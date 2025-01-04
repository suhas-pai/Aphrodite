/*
 * lib/overflow.h
 * © suhas pai
 */

#pragma once
#include "lib/assert.h"

#ifndef ckd_add
    #define ckd_add(result, lhs, rhs) \
        (!__builtin_add_overflow(lhs, rhs, result))
#endif /* ckd_add */

#ifndef ckd_mul
    #define ckd_mul(result, lhs, rhs) \
        (!__builtin_mul_overflow(lhs, rhs, result))
#endif /* ckd_mul */

#ifndef ckd_sub
    #define ckd_sub(result, lhs, rhs) \
        (!__builtin_sub_overflow(lhs, rhs, result))
#endif /* ckd_sub */

#define check_ptr_add(lhs, rhs, result) \
    (!__builtin_add_overflow((uint64_t)lhs, rhs, (uint64_t *)result))

#define check_ptr_sub(lhs, rhs, result) \
    (!__builtin_sub_overflow((uint64_t)lhs, rhs, (uint64_t *)result))

#define check_add_assert(lhs, rhs) ({ \
    __auto_type __check_add_assert_result__ = lhs; \
    assert(ckd_add(&__check_add_assert_result__, lhs, rhs)); \
    __check_add_assert_result__; \
})

#define check_sub_assert(lhs, rhs) ({ \
    __auto_type __check_sub_assert_result__ = lhs; \
    assert(ckd_sub(&__check_sub_assert_result__, lhs, rhs)); \
    __check_sub_assert_result__; \
})

#define check_mul_assert(lhs, rhs) ({ \
    __auto_type __check_mul_assert_result__ = lhs; \
    assert(ckd_mul(&__check_mul_assert_result__, lhs, rhs)); \
    __check_mul_assert_result__; \
})
