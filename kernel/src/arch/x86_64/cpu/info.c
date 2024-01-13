/*
 * kernel/src/arch/x86_64/cpu/info.c
 * © suhas pai
 */

#include "info.h"

struct cpu_info g_base_cpu_info = {
    .processor_id = 0,
    .lapic_id = 0,
    .lapic_timer_frequency = 0,
    .timer_ticks = 0,

    .process = &kernel_process,
    .pagemap_node = LIST_INIT(g_base_cpu_info.pagemap_node),

    .spur_int_count = 0
};

__optimize(3) const struct cpu_info *get_base_cpu_info() {
    return &g_base_cpu_info;
}
