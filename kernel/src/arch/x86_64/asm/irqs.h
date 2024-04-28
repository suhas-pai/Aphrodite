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

__optimize(3) static inline bool are_interrupts_enabled() {
    return read_rflags() & __RFLAGS_INTERRUPTS_ENABLED;
}

__optimize(3) static inline void disable_interrupts() {
    asm volatile ("cli");
}

__optimize(3) static inline void enable_interrupts() {
    asm volatile ("sti");
}

__optimize(3) static inline bool disable_irqs_if_enabled() {
    const bool result = are_interrupts_enabled();
    disable_interrupts();

    return result;
}

__optimize(3) static inline void enable_irqs_if_flag(const bool flag) {
    if (flag) {
        enable_interrupts();
    }
}
