/*
 * kernel/src/arch/aarch64/cpu/util.c
 * © suhas pai
 */

#include "cpu/info.h"
#include "cpu/panic.h"
#include "cpu/util.h"

#include "dev/psci.h"
#include "mm/kmalloc.h"

void cpu_idle() {
    while (true) {
        asm volatile ("wfi");
    }
}

void cpu_shutdown() {
    const enum psci_return_value result = psci_shutdown();
    panic("kernel: cpu_shutdown() failed with result=%d\n", result);
}

void cpu_reboot() {
    const enum psci_return_value result = psci_reboot();
    panic("kernel: cpu_reboot() failed with result=%d\n", result);
}

extern struct list g_cpu_list;

struct cpu_info *cpu_add(const struct limine_smp_info *const info) {
    struct cpu_info *const cpu = kmalloc(sizeof(*cpu));
    assert_msg(cpu != NULL, "cpu: failed to alloc info");

    list_init(&cpu->pagemap_node);
    list_init(&cpu->cpu_list);

    cpu->process = &kernel_process;
    cpu->spur_intr_count = 0;
    cpu->mpidr = info->mpidr;
    cpu->affinity =
        (((cpu->mpidr >> 32) & 0xFF) << 24) | (cpu->mpidr & 0xFFFFFF);

    cpu->processor_id = info->processor_id;

    cpu->spe_overflow_interrupt = 0;
    cpu->affinity = 0;

    list_add(&g_cpu_list, &cpu->cpu_list);
    return cpu;
}