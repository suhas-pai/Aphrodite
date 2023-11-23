/*
 * lib/assert.h
 * © suhas pai
 */

#pragma once
#include "macros.h"

#if defined(BUILD_KERNEL)
    #include "kernel/src/cpu/panic.h"

    #define assert(cond) \
        if (__builtin_expect(!(cond), 0)) panic(TO_STRING(cond) "\n")
    #define assert_msg(cond, msg, ...) \
        if (__builtin_expect(!(cond), 0)) panic(msg "\n", ##__VA_ARGS__)

    #if defined(DEBUG)
        #define verify_not_reached() panic("verify_not_reached()\n")
    #else
        #define verify_not_reached() __builtin_unreachable()
    #endif /* defined(DEBUG) */
#elif defined(BUILD_TEST)
    #include <assert.h>

    #define assert_msg(cond, msg, ...) assert(cond && (msg))
    #if defined(DEBUG)
        #define verify_not_reached() assert(0 && "verify_not_reached()")
    #else
        #define verify_not_reached() __builtin_unreachable()
    #endif /* defined(DEBUG) */
#else
    #include <assert.h>
#endif

#if !defined(BUILD_KERNEL)
    #define panic(msg, ...) assert(0 && (msg))
#endif /* !defined(BUILD_KERNEL) */
