/*
 * kernel/src/arch/riscv64/asm/context.h
 * © suhas pai
 */

#pragma once

#include "asm/status.h"
#include "stack_frame.h"

struct thread_context {
    uint64_t sepc;
    uint64_t sstatus;

    uint64_t sp;
    uint64_t gp;
    uint64_t tp;

    struct stack_frame frame;
};

#define THREAD_CONTEXT_INIT(thread, stack, stack_size, func, arg) \
    ((struct thread_context){ \
        .sepc = (uint64_t)(func), \
        .sstatus = \
            __SSTATUS_SUPERVISOR_PRIVL_INTR_ENABLE \
          | (((thread)->process == &kernel_process) ? \
                __SSTATUS_SUPERVISOR_PREV_PRIVL_WAS_SUPERVISOR : 0), \
        .sp = (uint64_t)(stack) + ((stack_size) - 1), \
        .gp = 0, \
        .tp = (uint64_t)(thread), \
        .frame = STACK_FRAME_INIT(arg), \
    })
