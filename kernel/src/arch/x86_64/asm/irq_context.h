/*
 * kernel/arch/x86_64/asm/irq_context.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

struct x86_64_irq_context {
    // Pushed by pushad
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx;
    uint64_t rax;

    // Interrupt number and error code (if applicable)
    uint64_t int_no, err_code;

    // Pushed by the processor automatically.
    uint64_t rip, cs, rflags, rsp, ss;
};

typedef struct x86_64_irq_context irq_context_t;