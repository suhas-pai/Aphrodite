/*
 * kernel/include/arch/x86_64/asm/tlb.h
 * © suhas pai
 */

#pragma once
#include "lib/macros.h"

__debug_optimize(3) static inline void invlpg(const uint64_t addr) {
    asm volatile ("invlpg (%0)" :: "r"(addr) : "memory");
}
