/*
 * kernel/src/arch/riscv64/asm/irq_context.h
 * © suhas pai
 */

#pragma once
#include "stack_frame.h"

struct irq_context {
    uint64_t sepc;
    uint64_t sstatus;

    struct stack_frame frame;
};

typedef struct irq_context irq_context_t;
