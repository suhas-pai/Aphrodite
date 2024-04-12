/*
 * kernel/src/arch/riscv64/sched/thread.c
 * © suhas pai
 */

#include "asm/csr.h"
#include "cpu/util.h"
#include "mm/pagemap.h"
#include "sched/thread.h"

__optimize(3) struct thread *current_thread() {
    return (struct thread *)csr_read(sscratch);
}

__optimize(3) void sched_set_current_thread(struct thread *const thread) {
    csr_write(sscratch, (uint64_t)thread);
}

void sched_prepare_thread(struct thread *const thread) {
    (void)thread;
}

extern void context_switch(struct stack_frame *prev, struct stack_frame *next);

__optimize(3) void
sched_switch_to(struct thread *const prev,
                struct thread *const next,
                struct thread_context *const prev_context)
{
    (void)prev_context;
    if (prev->process != next->process) {
        switch_to_pagemap(&next->process->pagemap);
    }

    context_switch(&prev->arch_info.frame, &next->arch_info.frame);
}

__optimize(3) void sched_switch_to_idle() {
    cpu_idle();
}