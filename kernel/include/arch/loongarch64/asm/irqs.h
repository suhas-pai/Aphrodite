/*
 * kernel/include/arch/loongarch64/asm/irqs.h
 * © suhas pai
 */

#pragma once
#include <lib/macros.h>

#include "csr.h"
#include "crmd.h"

__debug_optimize(3) static inline bool intr_are_enabled() {
    return csr_read(crmd) & __CRMD_INTR_ENABLE;
}

__debug_optimize(3) static inline void intr_disable() {
    csr_write(crmd, csr_read(crmd) | __CRMD_INTR_ENABLE);
}

__debug_optimize(3) static inline void intr_enable() {
    csr_write(crmd, rm_mask(csr_read(crmd), __CRMD_INTR_ENABLE));
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
        const bool h_var(irqs_disabled_flag) = intr_save(); \
        block; \
        intr_restore(h_var(irqs_disabled_flag)); \
    } while (false)
