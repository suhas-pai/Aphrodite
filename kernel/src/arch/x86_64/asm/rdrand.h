/*
 * kernel/src/arch/x86_64/asm/rdrand.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lib/macros.h"

__debug_optimize(3) static inline bool rdrand(uint64_t *const dest) {
    unsigned char ok;
    asm volatile ("rdrand %0; setc %1" : "=r" (*dest), "=qm" (ok) :: "cc");

    return ok;
}

__debug_optimize(3) static inline bool rdseed(uint64_t *const dest) {
    unsigned char ok;
    asm volatile ("rdseed %0; setc %1" : "=r" (*dest), "=qm" (ok) :: "cc");

    return ok;
}
