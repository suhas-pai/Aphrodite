/*
 * kernel/src/sched/init.c
 * © suhas pai
 */

#include "asm/irqs.h"
#include "cpu/util.h"
#include "mm/kmalloc.h"

#include "irq.h"
#include "scheduler.h"
#include "thread.h"

void sched_init() {
    assert(array_append(&kernel_process.threads, &kernel_main_thread));

    struct thread *const idle_thread = kmalloc(sizeof(struct thread));
    assert(idle_thread != NULL);

    const bool flag = disable_interrupts_if_not();

    sched_thread_init(idle_thread, &kernel_process, cpu_idle);
    g_base_cpu_info.idle_thread = idle_thread;

    sched_init_irq();
    sched_algo_init();

    sched_process_arch_info_init(&kernel_process);
    sched_process_algo_info_init(&kernel_process);

    sched_thread_init(&kernel_main_thread, &kernel_process, /*entry=*/NULL);
    sched_algo_post_init();

    enable_interrupts_if_flag(flag);
}