/*
 * kernel/src/arch/riscv64/asm/irqs.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include "status.h"

// ie = "Interrupt Enable"
enum ie_flags {
    __INTR_USER_SOFTWARE = 1ull << 0,
    __INTR_SUPERVISOR_SOFTWARE = 1ull << 1,
    __INTR_MACHINE_SOFTWARE = 1ull << 3,

    __INTR_USER_TIMER = 1ull << 4,
    __INTR_SUPERVISOR_TIMER = 1ull << 5,
    __INTR_MACHINE_TIMER = 1ull << 7,

    __INTR_USER_EXTERNAL = 1ull << 8,
    __INTR_SUPERVISOR_EXTERNAL = 1ull << 9,
    __INTR_MACHINE_EXTERNAL = 1ull << 11,

    __INTR_USER_ALL =
        __INTR_USER_SOFTWARE | __INTR_USER_TIMER | __INTR_USER_EXTERNAL,

    __INTR_SUPERVISOR_ALL =
        __INTR_SUPERVISOR_SOFTWARE
      | __INTR_SUPERVISOR_TIMER
      | __INTR_SUPERVISOR_EXTERNAL,

    __INTR_MACHINE_ALL =
        __INTR_MACHINE_SOFTWARE | __INTR_MACHINE_TIMER | __INTR_MACHINE_EXTERNAL
};

__debug_optimize(3) static inline void disable_interrupts(void) {
    asm volatile ("csrci sstatus, 0x2" ::: "memory");
}

__debug_optimize(3) static inline void enable_interrupts(void) {
    asm volatile ("csrsi sstatus, 0x2" ::: "memory");
}

__debug_optimize(3) static inline bool are_interrupts_enabled() {
    return csr_read(sstatus) & __SSTATUS_SUPERVISOR_INTR_ENABLE;
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

#define with_irqs_disabled(block) \
    do { \
        const bool h_var(irqs_disabled_flag) = disable_irqs_if_enabled(); \
        block; \
        enable_irqs_if_flag(h_var(irqs_disabled_flag)); \
    } while (false)

