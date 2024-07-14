/*
 * lib/math.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

bool round_up(uint64_t number, uint64_t multiple, uint64_t *result_out);
bool math_pow(uint64_t base, uint64_t exp, uint64_t *result_out);

#define round_up_assert(number, multiple) ({ \
    uint64_t h_var(round_up_result) = 0; \
    assert(round_up((number), (multiple), &h_var(round_up_result))); \
    h_var(round_up_result); \
})

#define math_pow_assert(base, exp) ({ \
    uint64_t h_var(math_pow_result) = 0; \
    assert(math_pow((base), (exp), &h_var(math_pow_result))); \
    h_var(math_pow_result); \
})
