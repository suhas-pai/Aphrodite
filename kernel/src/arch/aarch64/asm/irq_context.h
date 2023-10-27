/*
 * kernel/arch/aarch64/asm/irq_context.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

struct aarch64_irq_context {
    uint64_t x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14,
             x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27,
             x28, x29, x30, x31;

    uint64_t spsr_el1;
    uint64_t elr_el1;
    uint64_t sp_el0;
};

typedef struct aarch64_irq_context irq_context_t;