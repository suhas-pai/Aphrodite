/*
 * kernel/src/sched/init.c
 * © suhas pai
 */

#include "mm/kmalloc.h"

#include "irq.h"
#include "scheduler.h"
#include "thread.h"

void sched_init() {
    assert(array_append(&kernel_process.threads, &kernel_main_thread));

    struct thread *const idle_thread = kmalloc(sizeof(struct thread));
    assert(idle_thread != NULL);

    sched_thread_init(idle_thread, &kernel_process);
    g_base_cpu_info.idle_thread = idle_thread;

    sched_init_irq();
    sched_algo_init();

    sched_process_init(&kernel_process,
                       &kernel_pagemap,
                       STRING_STATIC("kernel"));

    sched_thread_init(&kernel_main_thread, &kernel_process);
}