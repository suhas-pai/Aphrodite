/*
 * kernel/src/arch/aarch64/asm/context.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "spsr.h"

struct thread_context {
    uint64_t x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14,
             x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27,
             x28, x29, x30;

    uint64_t sp_el1;
    uint64_t elr_el1;
    uint64_t spsr_el1;
    uint64_t esr_el1;
    uint64_t far_el1;
};

#define THREAD_CONTEXT_INIT(stack, stack_size, func, arg) \
    ((struct thread_context){ \
        .x0 = (uint64_t)(arg), \
        .x1 = 0, \
        .x2 = 0, \
        .x3 = 0, \
        .x4 = 0, \
        .x5 = 0, \
        .x6= 0, \
        .x7= 0, \
        .x8 = 0, \
        .x9 = 0, \
        .x10 = 0, \
        .x11 = 0, \
        .x12 = 0, \
        .x13 = 0, \
        .x14 = 0, \
        .x15 = 0, \
        .x16 = 0, \
        .x17 = 0, \
        .x18 = 0, \
        .x19 = 0, \
        .x20 = 0, \
        .x21 = 0, \
        .x22 = 0, \
        .x23 = 0, \
        .x24 = 0, \
        .x25 = 0, \
        .x26 = 0, \
        .x27 = 0, \
        .x28 = 0, \
        .x29 = 0, \
        .x30 = 0, \
        .sp_el1 = (uint64_t)(stack) + ((stack_size) - 1), \
        .elr_el1 = (uint64_t)(func), \
        .spsr_el1 = SPSR_MODE_EL1t, \
        .esr_el1 = 0, \
        .far_el1 = 0, \
    })

struct process;
void
thread_context_verify(const struct process *process,
                      const struct thread_context *context);
