/*
 * kernel/src/arch/aarch64/sched/thread.c
 * © suhas pai
 */

#include "cpu/util.h"
#include "sched/thread.h"

__optimize(3) struct thread *current_thread() {
    struct thread *thread = NULL;
    asm volatile ("mrs %0, tpidr_el1" : "=r"(thread));

    return thread;
}

__optimize(3) void sched_set_current_thread(struct thread *const thread) {
    asm volatile ("msr tpidr_el1, %0" :: "r"(thread));
}

void sched_prepare_thread(struct thread *const thread) {
    (void)thread;
}

extern __noreturn void thread_spinup(const struct thread_context *context);

void
sched_switch_to(struct thread *const prev,
                struct thread *const next,
                struct thread_context *const prev_context)
{
    (void)prev;
    (void)next;

    prev->context = *prev_context;
    thread_spinup(&next->context);

    verify_not_reached();
}

void sched_switch_to_idle() {
    cpu_idle();
}