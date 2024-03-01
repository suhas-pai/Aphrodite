/*
 * kernel/src/cpu/cpu_info.c
 * © suhas pai
 */

#include "asm/irqs.h"
#include "sched/thread.h"

__optimize(3) const struct cpu_info *this_cpu() {
    assert_msg(!are_interrupts_enabled(),
               "this_cpu() must be called with interrupts disabled");

    return current_thread()->cpu;
}

__optimize(3) struct cpu_info *this_cpu_mut() {
    assert_msg(!are_interrupts_enabled(),
               "this_cpu_mut() must be called with interrupts disabled");

    return current_thread()->cpu;
}