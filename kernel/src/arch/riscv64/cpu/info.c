/*
 * kernel/src/arch/riscv64/cpu/info.c
 * © suhas pai
 */

#include "mm/kmalloc.h"

#include "sched/process.h"
#include "sched/scheduler.h"

#include "sys/boot.h"
#include "info.h"

static struct cpu_info g_base_cpu_info = {
    CPU_INFO_BASE_INIT(g_base_cpu_info, &kernel_process),

    .cbo_size = 0,
    .cmo_size = 0,
    .hart_id = 0,
    .timer_start = 0,
    .is_active = true,
    .imsic_phys = 0,
    .imsic_page = NULL
};

struct cpus_info g_cpus_info = {
    .timebase_frequency = 0
};

__debug_optimize(3) void cpu_init() {
    this_cpu_mut()->hart_id = boot_get_smp()->bsp_hartid;
    list_add(cpus_get_list(), &this_cpu_mut()->cpu_list);
}

__debug_optimize(3) const struct cpu_info *base_cpu() {
    return &g_base_cpu_info;
}

__debug_optimize(3) uint32_t cpu_get_id(const struct cpu_info *const cpu) {
    return cpu->hart_id;
}

__debug_optimize(3) const struct cpu_info *cpu_for_id(const cpu_id_t hart_id) {
    const struct cpu_info *iter = NULL;
    list_foreach(iter, cpus_get_list(), cpu_list) {
        if (iter->hart_id == hart_id) {
            return iter;
        }
    }

    return NULL;
}

__debug_optimize(3) struct cpu_info *cpu_for_id_mut(const cpu_id_t hart_id) {
    struct cpu_info *iter = NULL;
    list_foreach(iter, cpus_get_list(), cpu_list) {
        if (iter->hart_id == hart_id) {
            return iter;
        }
    }

    return NULL;
}

__debug_optimize(3)
struct cpu_info *cpu_add(const struct limine_smp_info *const info) {
    struct cpu_info *const cpu = kmalloc(sizeof(*cpu));
    assert_msg(cpu != NULL, "cpu: failed to alloc cpu info");

    cpu_info_base_init(cpu, &kernel_process);

    cpu->spur_intr_count = 0;
    cpu->cbo_size = 0;
    cpu->cmo_size = 0;
    cpu->hart_id = info->hartid;
    cpu->is_active = false;

    sched_init_on_cpu(cpu);
    list_add(cpus_get_list(), &cpu->cpu_list);

    return cpu;
}

__debug_optimize(3) bool cpu_in_bad_state() {
    return this_cpu()->in_exception;
}

__debug_optimize(3) const struct cpus_info *get_cpus_info() {
    return &g_cpus_info;
}