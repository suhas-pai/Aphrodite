/*
 * kernel/include/arch/aarch64/asm/irqs.h
 * © suhas pai
 */

#pragma once
#include "lib/macros.h"

__debug_optimize(3) static inline void intr_disable(void) {
    asm volatile ("msr daifset, #15");
}

__debug_optimize(3) static inline void intr_enable(void) {
    asm volatile ("msr daifclr, #15");
}

__debug_optimize(3) static inline bool intr_are_enabled() {
    uint64_t value = 0;
    asm volatile ("mrs %0, daif" : "=r"(value));

    return value == 0;
}

__debug_optimize(3) static inline bool intr_save() {
    const bool result = intr_are_enabled();
    if (result) {
        intr_disable();
    }

    return result;
}

__debug_optimize(3) static inline void intr_restore(const bool flag) {
    if (flag) {
        intr_enable();
    }
}

#define with_intr_disabled(block) \
    do { \
        const bool h_var(irqs_disabled_flag) = intr_save(); \
        block; \
        intr_restore(h_var(irqs_disabled_flag)); \
    } while (false)
