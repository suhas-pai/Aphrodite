/*
 * kernel/src/arch/riscv64/asm/context.h
 * © suhas pai
 */

#pragma once
#include "stack_frame.h"

struct thread_context {
    uint64_t sepc;
    uint64_t sstatus;

    struct stack_frame frame;
};
