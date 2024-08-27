/*
 * kernel/src/arch/aarch64/sched/thread.c
 * © suhas pai
 */

#include "mm/pagemap.h"
#include "sched/thread.h"

__debug_optimize(3) struct thread *current_thread() {
    struct thread *thread = NULL;
    asm volatile ("mrs %0, tpidr_el1" : "=r"(thread));

    return thread;
}

__debug_optimize(3) void sched_set_current_thread(struct thread *const thread) {
    asm volatile ("msr tpidr_el1, %0" :: "r"(thread));
}

extern __noreturn void thread_spinup(const struct thread_context *context);

void
sched_save_restore_context(struct thread *const prev,
                           struct thread *const next,
                           struct thread_context *const prev_context)
{
    struct cpu_info *const cpu = next->cpu;
    if (prev != cpu->idle_thread) {
        prev->context = *prev_context;
    }

    thread_context_verify(next->process, &next->context);
}