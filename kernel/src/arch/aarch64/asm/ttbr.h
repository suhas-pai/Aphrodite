/*
 * kernel/arch/aarch64/asm/ttbr.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "lib/macros.h"

__optimize(3) static inline uint64_t read_ttbr0_el1() {
    uint64_t value = 0;
    asm volatile("mrs %0, ttbr0_el1" : "=r" (value));

    return (value & 0xffffffffffc0) | ((value & 0x3c) << 46);
}

__optimize(3) static inline uint64_t read_ttbr1_el1() {
    uint64_t value = 0;
    asm volatile("mrs %0, ttbr1_el1" : "=r" (value));

    return (value & 0xffffffffffc0) | ((value & 0x3c) << 46);
}

__optimize(3) static inline void write_ttbr0_el1(const uint64_t value) {
    asm volatile("msr ttbr0_el1, %0" :: "r" (value));
}

__optimize(3) static inline void write_ttbr1_el1(const uint64_t value) {
    asm volatile("msr ttbr1_el1, %0" :: "r" (value));
}