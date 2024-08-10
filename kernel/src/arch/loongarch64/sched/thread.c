/*
 * kernel/src/arch/aarch64/sched/thread.c
 * © suhas pai
 */

#include "mm/pagemap.h"
#include "sched/thread.h"

__debug_optimize(3) struct thread *current_thread() {
    struct thread *result = NULL;
    asm volatile ("move %0, $tp" : "=r"(result));

    return result;
}

__debug_optimize(3) void sched_set_current_thread(struct thread *const thread) {
    asm volatile ("move $tp, %0" :: "r"(thread));
}

extern __noreturn void thread_spinup(const struct thread_context *context);

void
sched_switch_to(struct thread *const prev,
                struct thread *const next,
                struct thread_context *const prev_context)
{
    if (prev->process != next->process) {
        switch_to_pagemap(&next->process->pagemap);
    }

    if (prev->cpu == NULL || prev != prev->cpu->idle_thread) {
        prev->context = *prev_context;
    }

    thread_spinup(&next->context);
    verify_not_reached();
}
