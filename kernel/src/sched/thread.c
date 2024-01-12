/*
 * kernel/src/sched/thread.c
 * © suhas pai
 */

#include "cpu/cpu_info.h"
#include "thread.h"

__hidden struct thread kernel_main_thread = {
    .process = &kernel_process,
    .cpu = &g_base_cpu_info,

    .premption_disabled = false,
    .signal_enqueued = false,

    .events_hearing = ARRAY_INIT(sizeof(struct event *)),
    .event_index = -1
};

void
sched_thread_init(struct thread *const thread, struct process *const process) {
    thread->process = process;
    thread->cpu = this_cpu_mut();

    thread->events_hearing = ARRAY_INIT(sizeof(struct event *));
    thread->premption_disabled = false;
    thread->signal_enqueued = false;

    thread->event_index = -1;

    sched_thread_arch_info_init(thread);
    sched_thread_algo_info_init(thread);
}

__optimize(3) void prempt_disable() {
    struct thread *const thread = current_thread();
    assert(thread != thread->cpu->idle_thread);

    thread->premption_disabled = true;
}

__optimize(3) void prempt_enable() {
    struct thread *const thread = current_thread();
    assert(thread != thread->cpu->idle_thread);

    thread->premption_disabled = false;
}