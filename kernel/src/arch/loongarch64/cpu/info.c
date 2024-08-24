/*
 * kernel/src/arch/loongarch64/cpu/info.c
 * © suhas pai
 */

#include "asm/csr.h"
#include "mm/kmalloc.h"
#include "sched/scheduler.h"

#include "info.h"

__hidden struct cpu_info g_base_cpu_info = {
    CPU_INFO_BASE_INIT(g_base_cpu_info, &kernel_process),

    .core_id = 0,
    .in_exception = false,
};

struct list g_cpu_list = LIST_INIT(g_cpu_list);
static bool g_base_cpu_init = false;

__debug_optimize(3) const struct cpu_info *base_cpu() {
    assert(g_base_cpu_init);
    return &g_base_cpu_info;
}

__debug_optimize(3) uint32_t cpu_get_id(const struct cpu_info *const cpu) {
    return cpu->core_id;
}

__debug_optimize(3) const struct cpu_info *cpu_for_id(const cpu_id_t id) {
    const struct cpu_info *iter = NULL;
    list_foreach(iter, cpus_get_list(), cpu_list) {
        if (iter->core_id == id) {
            return iter;
        }
    }

    return NULL;
}

__debug_optimize(3) struct cpu_info *cpu_for_id_mut(const cpu_id_t id) {
    struct cpu_info *iter = NULL;
    list_foreach(iter, cpus_get_list(), cpu_list) {
        if (iter->core_id == id) {
            return iter;
        }
    }

    return NULL;
}

__debug_optimize(3)
struct cpu_info *cpu_add(const struct limine_smp_info *const info) {
    (void)info;

    struct cpu_info *const cpu = kmalloc(sizeof(*cpu));
    assert_msg(cpu != NULL, "cpu: failed to alloc cpu info");

    cpu_info_base_init(cpu, &kernel_process);

    cpu->core_id = 0; // FIXME: Use info->core_id when added to limine
    cpu->in_exception = false;

    sched_init_on_cpu(cpu);
    list_add(cpus_get_list(), &cpu->cpu_list);

    return cpu;
}

extern void sched_set_current_thread(struct thread *thread);

void cpu_early_init() {
    sched_init_on_cpu(&g_base_cpu_info);
    sched_set_current_thread(&kernel_main_thread);
}

void cpu_init() {
    g_base_cpu_info.core_id = csr_read(cpuid);
    list_add(&g_cpu_list, &g_base_cpu_info.cpu_list);
}

__debug_optimize(3) bool cpu_in_bad_state() {
    return this_cpu()->in_exception;
}
