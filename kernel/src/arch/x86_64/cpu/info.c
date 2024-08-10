/*
 * kernel/src/arch/x86_64/cpu/info.c
 * © suhas pai
 */

#include "mm/kmalloc.h"
#include "info.h"

static struct cpu_info g_base_cpu_info = {
    .processor_id = 0,

    .lapic_id = 0,
    .lapic_timer_frequency = 0,
    .timer_ticks = 0,

    .active = true,
    .in_exception = false,

    .process = &kernel_process,
    .pagemap_node = LIST_INIT(g_base_cpu_info.pagemap_node),

    .spur_intr_count = 0
};

__debug_optimize(3) const struct cpu_info *base_cpu() {
    return &g_base_cpu_info;
}

__debug_optimize(3)
struct cpu_info *cpu_add(const struct limine_smp_info *const info) {
    struct cpu_info *const cpu = kmalloc(sizeof(*cpu));
    assert_msg(cpu != NULL, "cpu_add(): failed to alloc cpu");

    list_init(&cpu->pagemap_node),

    cpu->processor_id = info->processor_id;
    cpu->lapic_id = info->lapic_id;
    cpu->lapic_timer_frequency = 0;
    cpu->timer_ticks = 0;

    cpu->process = &kernel_process;
    cpu->spur_intr_count = 0;

    return cpu;
}

__debug_optimize(3) bool cpu_in_bad_state() {
    return this_cpu()->in_exception;
}