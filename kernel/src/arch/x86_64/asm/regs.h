/*
 * kernel/arch/x86_64/asm/regs.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "lib/macros.h"

__optimize(3) static inline uint64_t read_cr3() {
    uint64_t cr3 = 0;
    asm volatile ("mov %%cr3, %0" : "=r"(cr3));

    return cr3;
}

__optimize(3) static inline void write_cr3(const uint64_t cr3) {
    asm volatile ("mov %0, %%cr3" :: "r"(cr3) : "memory");
}
