/*
 * kernel/src/arch/riscv64/cpu/info.c
 * © suhas pai
 */

#include "sched/process.h"
#include "info.h"

static struct cpu_info g_base_cpu_info = {
    .process = &kernel_process,
    .pagemap_node = LIST_INIT(g_base_cpu_info.pagemap_node),
    .spur_int_count = 0
};

struct cpus_info g_cpus_info = {
    .timebase_frequency = 0
};

__optimize(3) const struct cpu_info *get_base_cpu_info() {
    return &g_base_cpu_info;
}

__optimize(3) const struct cpus_info *get_cpus_info() {
    return &g_cpus_info;
}