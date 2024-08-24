/*
 * kernel/src/sched/init.c
 * © suhas pai
 */

#include "asm/irqs.h"
#include "cpu/util.h"
#include "mm/kmalloc.h"

#include "irq.h"
#include "scheduler.h"

void sched_init() {
    assert(array_append(&kernel_process.threads, &kernel_main_thread));

    sched_init_on_cpu(&g_base_cpu_info);
    sched_init_irq();

    const bool flag = disable_irqs_if_enabled();
    sched_algo_init();

    sched_process_arch_info_init(&kernel_process);
    sched_process_algo_info_init(&kernel_process);

    sched_thread_init(&kernel_main_thread,
                      &kernel_process,
                      this_cpu_mut(),
                      /*entry=*/NULL);

    sched_algo_post_init();
    enable_irqs_if_flag(flag);
}

void sched_init_on_cpu(struct cpu_info *const cpu) {
    struct thread *const idle_thread = kmalloc(sizeof(struct thread));
    assert(idle_thread != NULL);

    const bool flag = disable_irqs_if_enabled();

    sched_thread_init(idle_thread, &kernel_process, cpu, cpu_idle);
    cpu->idle_thread = idle_thread;

    enable_irqs_if_flag(flag);
}