/*
 * kernel/src/arch/aarch64/asm/irqs.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include "lib/macros.h"

__debug_optimize(3) static inline void disable_interrupts(void) {
    asm volatile ("msr daifset, #15");
}

__debug_optimize(3) static inline void enable_interrupts(void) {
    asm volatile ("msr daifclr, #15");
}

__debug_optimize(3) static inline bool are_interrupts_enabled() {
    uint64_t value = 0;
    asm volatile ("mrs %0, daif" : "=r"(value));

    return value == 0;
}

__debug_optimize(3) static inline bool disable_irqs_if_enabled() {
    const bool result = are_interrupts_enabled();
    if (result) {
        disable_interrupts();
    }

    return result;
}

__debug_optimize(3) static inline void enable_irqs_if_flag(const bool flag) {
    if (flag) {
        enable_interrupts();
    }
}
