/*
 * kernel/src/arch/riscv64/asm/irqs.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lib/macros.h"

__optimize(3) static inline void disable_all_irqs(void) {
    asm volatile ("csrci sstatus, 0x2" ::: "memory");
}

__optimize(3) static inline void enable_all_irqs(void) {
    asm volatile ("csrsi sstatus, 0x2" ::: "memory");
}

__optimize(3) static inline bool are_irqs_enabled() {
    uint64_t info = 0;
    asm volatile ("csrr %0, sstatus" : "=r" (info));

    return !(info & 0x2);
}

__optimize(3) static inline bool disable_all_irqs_if_not() {
    const bool result = are_irqs_enabled();
    disable_all_irqs();

    return result;
}

__optimize(3) static inline void enable_all_irqs_if_flag(const bool flag) {
    if (flag) {
        enable_all_irqs();
    }
}
