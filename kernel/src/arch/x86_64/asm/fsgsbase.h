/*
 * kernel/arch/x86_64/asm/fsgsbase.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "lib/macros.h"

__optimize(3) static inline uint64_t read_fsbase() {
    uint64_t result = 0;
    asm volatile ("rdfsbase %0" : "=r"(result));

    return result;
}

__optimize(3) static inline uint64_t read_gsbase() {
    uint64_t result = 0;
    asm volatile ("rdgsbase %0" : "=r"(result));

    return result;
}

__optimize(3) static inline void write_fsbase(const uint64_t fs_base) {
    asm volatile ("wrfsbase %0" :: "r"(fs_base) : "memory");
}

__optimize(3) static inline void write_gsbase(const uint64_t gs_base) {
    asm volatile ("wrgsbase %0" :: "r"(gs_base) : "memory");
}
