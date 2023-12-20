/*
 * kernel/src/sched/init.c
 * © suhas pai
 */

#include "cpu/cpu_info.h"
#include "cpu/isr.h"

#include "mm/kmalloc.h"

#include "sched/process.h"
#include "sched/thread.h"

__hidden isr_vector_t g_sched_vector = 0;

void sched_init() {
    g_sched_vector = isr_alloc_vector();
    assert(array_append(&kernel_process.threads, &kernel_main_thread));

    struct thread *const idle_thread = kmalloc(sizeof(struct thread));
    assert(idle_thread != NULL);

    idle_thread->cpu = get_cpu_info_mut();
    idle_thread->process = &kernel_process;

    g_base_cpu_info.idle_thread = idle_thread;
}