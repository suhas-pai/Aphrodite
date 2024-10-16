/*
 * kernel/src/arch/x86_64/asm/rflags.h
 * © suhas pai
 */

#pragma once
#include "lib/macros.h"

enum rflags {
    __RFLAGS_CARRY = 1 << 0,
    __RFLAGS_PARITY = 1 << 2,

    // Auxilary carry
    __RFLAGS_ADJUST = 1 << 4,
    __RFLAGS_ZERO = 1 << 6,
    __RFLAGS_SIGN = 1 << 7,

    // Single-Step Interrupts
    __RFLAGS_TRAP = 1 << 8,
    __RFLAGS_INTERRUPTS_ENABLED = 1 << 9,

    // String-processing direction
    __RFLAGS_DIRECTION = 1 << 10,
    __RFLAGS_OVERFLOW = 1 << 11,

    __RFLAGS_IO_PRIVILEGE_LEVEL = 0b11 << 12,
    __RFLAGS_RESUME = 1 << 16,
    __RFLAGS_VIRTUAL_8086_MODE = 1 << 17,
    __RFLAGS_ALIGNMENT_CHECK = 1 << 18,

    __RFLAGS_VIRTUAL_INTERRUPT_FLAG = 1 << 19,
    __RFLAGS_VIRTUAL_INTERRUPT_PENDING = 1 << 20,

    // cpuid is supported if this bit can be modified.
    __RFLAGS_CPUID_BIT = 1 << 21
};

__debug_optimize(3) static inline uint64_t rflags_read() {
    uint64_t rflags = 0;
    asm volatile ("pushfq\n"
                  "popq %0" : "=r" (rflags) :: "memory");

    return rflags;
}

__debug_optimize(3) static inline void rflags_write(const uint64_t rflags) {
    asm volatile ("push %0\n"
                  "popfq" :: "r" (rflags) : "memory");
}
