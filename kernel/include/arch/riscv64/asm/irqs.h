/*
 * kernel/include/arch/riscv64/asm/irqs.h
 * © suhas pai
 */

#pragma once
#include "status.h"

// ie = "Interrupt Enable"
enum ie_flags : uint16_t {
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

__debug_optimize(3) static inline void intr_disable(void) {
    asm volatile ("csrci sstatus, 0x2" ::: "memory");
}

__debug_optimize(3) static inline void intr_enable(void) {
    asm volatile ("csrsi sstatus, 0x2" ::: "memory");
}

__debug_optimize(3) static inline bool intr_are_enabled() {
    return csr_read(sstatus) & __SSTATUS_SUPERVISOR_INTR_ENABLE;
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
