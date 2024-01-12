/*
 * kernel/src/arch/aarch64/asm/irqs.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include "lib/macros.h"

__optimize(3) static inline void disable_interrupts(void) {
    asm volatile ("msr daifset, #15");
}

__optimize(3) static inline void enable_interrupts(void) {
    asm volatile ("msr daifclr, #15");
}

__optimize(3) static inline bool are_interrupts_enabled() {
    return true;
}

__optimize(3) static inline bool disable_interrupts_if_not() {
    const bool result = are_interrupts_enabled();
    disable_interrupts();

    return result;
}

__optimize(3) static inline void enable_interrupts_if_flag(const bool flag) {
    if (flag) {
        enable_interrupts();
    }
}
