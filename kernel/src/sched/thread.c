/*
 * kernel/src/sched/thread.c
 * © suhas pai
 */

#include "asm/irqs.h"
#include "cpu/cpu_info.h"
#include "thread.h"

__hidden struct thread kernel_main_thread = {
    .process = &kernel_process,
    .cpu = &g_base_cpu_info,

    .preemption_disabled = false,
    .signal_enqueued = false,

    .event_index = -1
};

void
sched_thread_init(struct thread *const thread,
                  struct process *const process,
                  const void *const entry)
{
    thread->process = process;
    thread->cpu = this_cpu_mut();

    thread->preemption_disabled = false;
    thread->signal_enqueued = false;

    thread->event_index = -1;

    sched_thread_arch_info_init(thread, entry);
    sched_thread_algo_info_init(thread);
}

__optimize(3) void preempt_disable() {
    const bool flag = disable_interrupts_if_not();

    struct thread *const thread = current_thread();
    assert(thread != thread->cpu->idle_thread);

    thread->preemption_disabled = true;
    enable_interrupts_if_flag(flag);
}

__optimize(3) void preempt_enable() {
    const bool flag = disable_interrupts_if_not();

    struct thread *const thread = current_thread();
    assert(thread != thread->cpu->idle_thread);

    thread->preemption_disabled = false;
    enable_interrupts_if_flag(flag);
}