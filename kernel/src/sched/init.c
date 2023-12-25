/*
 * kernel/src/sched/init.c
 * © suhas pai
 */

#include "cpu/cpu_info.h"
#include "mm/kmalloc.h"

#include "irq.h"
#include "process.h"
#include "scheduler.h"
#include "thread.h"

void sched_init(struct scheduler *const sched) {
    (void)sched;
    assert(array_append(&kernel_process.threads, &kernel_main_thread));

    struct thread *const idle_thread = kmalloc(sizeof(struct thread));
    assert(idle_thread != NULL);

    idle_thread->cpu = this_cpu_mut();
    idle_thread->process = &kernel_process;

    g_base_cpu_info.idle_thread = idle_thread;
    sched_init_irq();
}