/*
 * kernel/src/arch/x86_64/asm/irqs.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include "rflags.h"

enum irq_number {
    IRQ_TIMER = 0,
    IRQ_KEYBOARD = 1,
};

__debug_optimize(3) static inline bool are_interrupts_enabled() {
    return rflags_read() & __RFLAGS_INTERRUPTS_ENABLED;
}

__debug_optimize(3) static inline void disable_interrupts() {
    asm volatile ("cli");
}

__debug_optimize(3) static inline void enable_interrupts() {
    asm volatile ("sti");
}

__debug_optimize(3) static inline bool disable_irqs_if_enabled() {
    const bool result = are_interrupts_enabled();
    disable_interrupts();

    return result;
}

__debug_optimize(3) static inline void enable_irqs_if_flag(const bool flag) {
    if (flag) {
        enable_interrupts();
    }
}

#define with_irqs_disabled(block) \
    do { \
        const bool h_var(irqs_disabled_flag) = disable_irqs_if_enabled(); \
        block; \
        enable_irqs_if_flag(h_var(irqs_disabled_flag)); \
    } while (false)
