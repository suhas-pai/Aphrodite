/*
 * kernel/arch/aarch64/asm/mair.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "lib/macros.h"

__optimize(3) static inline uint64_t read_mair_el1() {
    uint64_t value = 0;
    asm volatile ("mrs %0, mair_el1" : "=r"(value));

    return value;
}

__optimize(3) static inline void write_mair_el1(const uint64_t value) {
    asm volatile ("msr mair_el1, %0" :: "r"(value));
}