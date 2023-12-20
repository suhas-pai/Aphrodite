/*
 * kernel/src/sched/thread.c
 * © suhas pai
 */

#include "cpu/cpu_info.h"
#include "thread.h"

__hidden struct thread kernel_main_thread = {
    .process = &kernel_process,
    .cpu = &g_base_cpu_info,
    .sched_info = SCHED_THREAD_INFO_INIT()
};