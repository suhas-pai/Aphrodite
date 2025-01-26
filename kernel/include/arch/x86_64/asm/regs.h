/*
 * kernel/include/arch/x86_64/asm/regs.h
 * © suhas pai
 */

#pragma once
#include <lib/macros.h>

__debug_optimize(3) static inline uint64_t cr3_read() {
    uint64_t cr3 = 0;
    asm volatile ("mov %%cr3, %0" : "=r"(cr3));

    return cr3;
}

__debug_optimize(3) static inline void cr3_write(const uint64_t cr3) {
    asm volatile ("mov %0, %%cr3" :: "r"(cr3) : "memory");
}
