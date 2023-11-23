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

__optimize(3) static inline bool are_irqs_enabled() {
    return read_rflags() & __RFLAGS_INTERRUPTS_ENABLED;
}

__optimize(3) static inline void disable_all_irqs() {
    asm volatile ("cli");
}

__optimize(3) static inline void enable_all_irqs() {
    asm volatile ("sti");
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


