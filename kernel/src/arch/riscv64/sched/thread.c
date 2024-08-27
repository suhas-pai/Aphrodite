/*
 * kernel/src/arch/riscv64/sched/thread.c
 * © suhas pai
 */

#include "mm/kmalloc.h"
#include "sched/thread.h"

__debug_optimize(3) struct thread *current_thread() {
    struct thread *x = NULL;
    asm volatile("mv %0, tp" : "=r"(x));

    return x;
}

__debug_optimize(3) void sched_set_current_thread(struct thread *const thread) {
    asm volatile("mv tp, %0" : : "r"((uint64_t)thread));
}

extern __noreturn void thread_spinup(const struct thread_context *context);

__debug_optimize(3) void
sched_save_restore_context(struct thread *const prev,
                           struct thread *const next,
                           struct thread_context *const prev_context)
{
    if (prev->cpu == NULL || prev != prev->cpu->idle_thread) {
        prev->context = *prev_context;
    }

    thread_context_verify(next->process, &next->context);
}