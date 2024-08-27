/*
 * kernel/src/sched/thread.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "asm/irqs.h"
#include "cpu/info.h"

#include "thread.h"

__hidden struct thread kernel_main_thread = {
    .process = &kernel_process,
    .cpu = &g_base_cpu_info,

    .preemption_disabled = 0,
    .signal_enqueued = false,

    .event_index = -1,
};

void
sched_thread_init(struct thread *const thread,
                  struct process *const process,
                  struct cpu_info *const cpu,
                  const void *const entry)
{
    thread->process = process;
    thread->cpu = cpu;

    thread->preemption_disabled = 0;
    thread->signal_enqueued = false;

    thread->event_index = -1;

    sched_thread_arch_info_init(thread, entry);
    sched_thread_algo_info_init(thread);
}

__debug_optimize(3) bool preemption_enabled() {
    return current_thread()->preemption_disabled == 0;
}

__debug_optimize(3) void preempt_disable() {
    atomic_fetch_add_explicit(&current_thread()->preemption_disabled,
                              1,
                              memory_order_relaxed);
}

__debug_optimize(3) void preempt_enable() {
    const uint16_t old =
        atomic_fetch_sub_explicit(&current_thread()->preemption_disabled,
                                  1,
                                  memory_order_relaxed);

    assert(old != 0);
}