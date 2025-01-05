/*
 * kernel/src/arch/x86_64/asm/irqs.h
 * © suhas pai
 */

#pragma once
#include "rflags.h"

enum irq_number {
    IRQ_TIMER = 0,
    IRQ_KEYBOARD = 1,
};

__debug_optimize(3) static bool intr_are_enabled() {
    return rflags_read() & __RFLAGS_INTERRUPTS_ENABLED;
}

__debug_optimize(3) static inline void intr_disable() {
    asm volatile ("cli");
}

__debug_optimize(3) static inline void intr_enable() {
    asm volatile ("sti");
}

__debug_optimize(3) static inline bool intr_save() {
    const bool result = intr_are_enabled();
    intr_disable();

    return result;
}

__debug_optimize(3) static inline void intr_restore(const bool flag) {
    if (flag) {
        intr_enable();
    }
}

#define with_intr_disabled(block) \
    do { \
        const bool h_var(intr_is_disabled) = intr_save(); \
        block; \
        intr_restore(h_var(intr_is_disabled)); \
    } while (false)
