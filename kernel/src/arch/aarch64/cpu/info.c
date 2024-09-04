/*
 * kernel/src/arch/aarch64/cpu/info.c
 * © suhas pai
 */

#include "sys/gic/v2.h"
#include "cpu/cpu_info.h"

#include "info.h"

__hidden struct cpu_info g_base_cpu_info = {
    CPU_INFO_BASE_INIT(g_base_cpu_info),

    .mpidr = 0,
    .processor_id = 0,
    .affinity = 0,

    .spe_overflow_interrupt = 0,

    .gic_its_pend_page = NULL,
    .gic_its_prop_page = NULL,

    .in_lpi = false,
    .in_exception = false,
};

static bool g_base_cpu_init = false;

__debug_optimize(3) const struct cpu_info *base_cpu() {
    assert(g_base_cpu_init);
    return &g_base_cpu_info;
}

__debug_optimize(3) uint32_t cpu_get_id(const struct cpu_info *const cpu) {
    return cpu->processor_id;
}

__debug_optimize(3) const struct cpu_info *cpu_for_id(const cpu_id_t id) {
    const struct cpu_info *iter = NULL;
    list_foreach(iter, cpus_get_list(), cpu_list) {
        if (iter->processor_id == id) {
            return iter;
        }
    }

    return NULL;
}

__debug_optimize(3) struct cpu_info *cpu_for_id_mut(const cpu_id_t id) {
    struct cpu_info *iter = NULL;
    list_foreach(iter, cpus_get_list(), cpu_list) {
        if (iter->processor_id == id) {
            return iter;
        }
    }

    return NULL;
}

__debug_optimize(3) bool cpu_in_bad_state() {
    return this_cpu()->in_exception;
}
