/*
 * kernel/src/sched/thread.c
 * © suhas pai
 */

#include "cpu/cpu_info.h"
#include "thread.h"

__hidden struct thread kernel_main_thread = {
    .process = &kernel_process,
    .cpu = &g_base_cpu_info,
    .events_hearing = ARRAY_INIT(sizeof(struct event *)),
    .sched_info = SCHED_THREAD_INFO_INIT(),
    .premption_disabled = false
};

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