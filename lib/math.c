/*
 * lib/math.c
 * © suhas pai
 */

#include "lib/overflow.h"
#include "math.h"

__debug_optimize(3) bool
round_up(const uint64_t number,
         const uint64_t multiple,
         uint64_t *const result_out)
{
    assert(multiple != 0);
    if (number == 0) {
        *result_out = multiple;
        return true;
    }

    if (!ckd_add(result_out, number, multiple - 1)
     || !ckd_mul(result_out, *result_out / multiple, multiple))
    {
        return false;
    }

    return true;
}

__debug_optimize(3) bool
math_pow(const uint64_t base, const uint64_t exp, uint64_t *const result_out) {
    uint64_t result = base;
    for (uint64_t i = 0; i != exp; i++) {
        if (!ckd_mul(&result, result, base)) {
            return false;
        }
    }

    *result_out = result;
    return true;
}