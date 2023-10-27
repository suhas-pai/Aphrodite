/*
 * kernel/arch/riscv64/asm/irqs.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lib/macros.h"

__optimize(3) static inline void disable_all_interrupts(void) {
    asm volatile ("csrci sstatus, 0x2" ::: "memory");
}

__optimize(3) static inline void enable_all_interrupts(void) {
    asm volatile ("csrsi sstatus, 0x2" ::: "memory");
}

__optimize(3) static inline bool are_interrupts_enabled() {
    uint64_t info = 0;
    asm volatile("csrr %0, sstatus" : "=r" (info));

    return !(info & 0x2);
}