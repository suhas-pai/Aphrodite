/*
 * kernel/src/arch/aarch64/cpu/util.c
 * © suhas pai
 */

#include "asm/irqs.h"
#include "cpu/util.h"

#include "dev/psci.h"
#include "mm/kmalloc.h"

#include "sched/scheduler.h"

__noreturn void cpu_idle() {
    assert(are_interrupts_enabled());
    cpu_halt();
}

__noreturn void cpu_halt() {
    while (true) {
        asm ("wfi");
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

struct cpu_info *cpu_add(const struct limine_smp_info *const info) {
    struct cpu_info *const cpu = kmalloc(sizeof(*cpu));
    assert_msg(cpu != NULL, "cpu: failed to alloc info");

    cpu_info_base_init(cpu);

    cpu->mpidr = info->mpidr;
    cpu->processor_id = info->processor_id;
    cpu->affinity =
        ((cpu->mpidr >> 32) & 0xFF) << 24 | (cpu->mpidr & 0xFFFFFF);

    cpu->spe_overflow_interrupt = 0;
    cpu->icid = 0;

    cpu->gic_its_pend_page = NULL;
    cpu->gic_its_prop_page = NULL;

    cpu->in_lpi = false;
    cpu->in_exception = false;

    sched_init_on_cpu(cpu);
    list_add(cpus_get_list(), &cpu->cpu_list);

    return cpu;
}