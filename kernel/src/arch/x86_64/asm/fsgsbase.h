/*
 * kernel/src/arch/x86_64/asm/fsgsbase.h
 * © suhas pai
 */

#pragma once
#include "lib/macros.h"

__debug_optimize(3) static inline uint64_t fsbase_read() {
    uint64_t result = 0;
    asm volatile ("rdfsbase %0" : "=r"(result));

    return result;
}

__debug_optimize(3) static inline uint64_t gsbase_read() {
    uint64_t result = 0;
    asm volatile ("rdgsbase %0" : "=r"(result));

    return result;
}

__debug_optimize(3) static inline void fsbase_write(const uint64_t fs_base) {
    asm volatile ("wrfsbase %0" :: "r"(fs_base) : "memory");
}

__debug_optimize(3) static inline void gsbase_write(const uint64_t gs_base) {
    asm volatile ("wrgsbase %0" :: "r"(gs_base) : "memory");
}
