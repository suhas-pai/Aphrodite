/*
 * kernel/src/cpu/cpu_info.c
 * © suhas pai
 */

#include "asm/irqs.h"
#include "sched/thread.h"

static struct list g_cpu_list = LIST_INIT(g_cpu_list);

__debug_optimize(3) const struct cpu_info *this_cpu() {
    assert_msg(!(are_interrupts_enabled() && preemption_enabled()),
               "this_cpu() must be called with interrupts disabled or with "
               "preemption disabled");

    return current_thread()->cpu;
}

__debug_optimize(3) struct cpu_info *this_cpu_mut() {
    assert_msg(!(are_interrupts_enabled() && preemption_enabled()),
               "this_cpu_mut() must be called with interrupts disabled or "
               "with preemption disabled");

    return current_thread()->cpu;
}

__debug_optimize(3) void
cpu_info_base_init(struct cpu_info *const cpu, struct process *const process) {
    list_init(&cpu->alarm_list);
    list_init(&cpu->cpu_list);
    list_init(&cpu->pagemap_node);

    cpu->process = process;
    cpu->idle_thread = NULL;

    cpu->spur_intr_count = 0;
    cpu->sched_info = SCHED_PERCPU_INFO_INIT();
}

__debug_optimize(3) struct list *cpus_get_list() {
    return &g_cpu_list;
}