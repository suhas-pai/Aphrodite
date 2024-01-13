/*
 * kernel/src/arch/riscv64/asm/irqs.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include "status.h"

// ip = "Interrupt Pending"
enum ip_flags {
    __IP_USER_SW_INT_PENDING = 1ull << 0,
    __IP_SUPERVISOR_SW_INT_PENDING = 1ull << 1,
    __IP_MACHINE_SW_INT_PENDING = 1ull << 3,

    __IP_USER_TIMER_INT_PENDING = 1ull << 4,
    __IP_SUPERVISOR_TIMER_INT_PENDING = 1ull << 5,
    __IP_MACHINE_TIMER_INT_PENDING = 1ull << 7,

    __IP_USER_EXT_INT_PENDING = 1ull << 8,
    __IP_SUPERVISOR_EXT_INT_PENDING = 1ull << 9,
    __IP_MACHINE_EXT_INT_PENDING = 1ull << 11,
};

// ie = "Interrupt Enable"
enum ie_flags {
    __IE_USER_SW_INT_ENABLE = 1ull << 0,
    __IE_SUPERVISOR_SW_INT_ENABLE = 1ull << 1,
    __IE_MACHINE_SW_INT_ENABLE = 1ull << 3,

    __IE_USER_TIMER_INT_ENABLE = 1ull << 4,
    __IE_SUPERVISOR_TIMER_INT_ENABLE = 1ull << 5,
    __IE_MACHINE_TIMER_INT_ENABLE = 1ull << 7,

    __IE_USER_EXT_INT_ENABLE = 1ull << 8,
    __IE_SUPERVISOR_EXT_INT_ENABLE = 1ull << 9,
    __IE_MACHINE_EXT_INT_ENABLE = 1ull << 11,
};

__optimize(3) static inline void disable_interrupts(void) {
    asm volatile ("csrci sstatus, 0x2" ::: "memory");
}

__optimize(3) static inline void enable_interrupts(void) {
    asm volatile ("csrsi sstatus, 0x2" ::: "memory");
}

__optimize(3) static inline bool are_interrupts_enabled() {
    return read_sstatus() & __SSTATUS_SUPERVISOR_INT_ENABLE;
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
