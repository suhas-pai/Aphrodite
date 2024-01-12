/*
 * kernel/src/cpu/cpu_info.c
 * © suhas pai
 */

#include "sched/thread.h"

__optimize(3) const struct cpu_info *this_cpu() {
    return current_thread()->cpu;
}

__optimize(3) struct cpu_info *this_cpu_mut() {
    return current_thread()->cpu;
}