/*
 * kernel/src/arch/aarch64/asm/irqs.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include "lib/macros.h"

__optimize(3) static inline void disable_all_irqs(void) {
    asm volatile ("msr daifset, #15");
}

__optimize(3) static inline void enable_all_irqs(void) {
    asm volatile ("msr daifclr, #15");
}

__optimize(3) static inline bool are_irqs_enabled() {
    return true;
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
