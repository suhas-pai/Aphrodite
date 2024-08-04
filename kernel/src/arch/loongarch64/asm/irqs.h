/*
 * kernel/src/arch/loongarch64/asm/irqs.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include "lib/macros.h"

#include "csr.h"
#include "crmd.h"

__optimize(3) static inline bool are_interrupts_enabled() {
    return csr_read(crmd) & __CRMD_INTR_ENABLE;
}

__optimize(3) static inline void disable_interrupts() {
    csr_write(crmd, csr_read(crmd) | __CRMD_INTR_ENABLE);
}

__optimize(3) static inline void enable_interrupts() {
    csr_write(crmd, rm_mask(csr_read(crmd), __CRMD_INTR_ENABLE));
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
