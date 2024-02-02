/*
 * kernel/src/arch/x86_64/asm/irq_context.h
 * © suhas pai
 */

#pragma once
#include "asm/context.h"

struct x86_64_irq_context {
    // Pushed by pushad
    struct register_context regs;
    uint64_t err_code;

    // Pushed by the processor automatically.
    uint64_t rip, cs, rflags, rsp, ss;
};

typedef struct x86_64_irq_context irq_context_t;
