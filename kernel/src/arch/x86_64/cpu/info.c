/*
 * kernel/src/arch/x86_64/cpu/info.c
 * © suhas pai
 */

#include "mm/kmalloc.h"

#include "sched/process.h"
#include "sched/scheduler.h"

#include "info.h"

static struct cpu_info g_base_cpu_info = {
    CPU_INFO_BASE_INIT(g_base_cpu_info, &kernel_process),

    .processor_id = 0,
    .lapic_id = 0,
    .lapic_timer_frequency = 0,

    .active = true,
    .in_exception = false,
};

__debug_optimize(3) const struct cpu_info *base_cpu() {
    return &g_base_cpu_info;
}

__debug_optimize(3) uint32_t cpu_get_id(const struct cpu_info *const cpu) {
    return cpu->lapic_id;
}

__debug_optimize(3) const struct cpu_info *cpu_for_id(const cpu_id_t lapic_id) {
    const struct cpu_info *iter = NULL;
    list_foreach(iter, cpus_get_list(), cpu_list) {
        if (iter->lapic_id == lapic_id) {
            return iter;
        }
    }

    return NULL;
}

__debug_optimize(3) struct cpu_info *cpu_for_id_mut(const cpu_id_t lapic_id) {
    struct cpu_info *iter = NULL;
    list_foreach(iter, cpus_get_list(), cpu_list) {
        if (iter->lapic_id == lapic_id) {
            return iter;
        }
    }

    return NULL;
}

__debug_optimize(3)
struct cpu_info *cpu_add(const struct limine_smp_info *const info) {
    struct cpu_info *const cpu = kmalloc(sizeof(*cpu));
    assert_msg(cpu != NULL, "cpu_add(): failed to alloc cpu");

    cpu_info_base_init(cpu, &kernel_process);

    cpu->processor_id = info->processor_id;
    cpu->lapic_id = info->lapic_id;
    cpu->lapic_timer_frequency = 0;

    sched_init_on_cpu(cpu);
    list_add(cpus_get_list(), &cpu->cpu_list);

    return cpu;
}

__debug_optimize(3) bool cpu_in_bad_state() {
    return this_cpu()->in_exception;
}