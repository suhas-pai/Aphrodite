/*
 * kernel/src/arch/x86_64/asm/context.h
 * © suhas pai
 */

#pragma once
#include "rflags.h"

struct thread_context {
    // Pushed by pushad
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx;
    uint64_t rax;

    uint64_t err_code;

    // Pushed by the processor automatically.
    uint64_t rip, cs, rflags, rsp, ss;
};

#define THREAD_CONTEXT_INIT(process, stack, func, arg) \
    ((struct thread_context){ \
        .r15 = 0, \
        .r14 = 0, \
        .r13 = 0, \
        .r12 = 0, \
        .r11 = 0, \
        .r10 = 0, \
        .r9 = 0, \
        .r8 = 0, \
        .rbp = 0, \
        .rdi = (uint64_t)(arg), \
        .rsi = 0, \
        .rdx = 0, \
        .rcx = 0, \
        .rbx = 0, \
        .rax = 0, \
        .err_code = 0, \
        .rip = (uint64_t)(func), \
        .cs = (process == &kernel_process) ? 0x4b : 0x28, \
        .rflags = __RFLAGS_INTERRUPTS_ENABLED, \
        .rsp = (uint64_t)(stack), \
        .ss = 0x30, \
    })
