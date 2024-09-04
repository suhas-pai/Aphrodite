/*
 * kernel/src/arch/aarch64/init.c
 * © suhas pai
 */

#include <stdatomic.h>
#include "sys/gic/api.h"

#include "acpi/api.h"
#include "asm/ttbr.h"

#include "cpu/init.h"
#include "cpu/smp.h"
#include "cpu/util.h"

#include "mm/early.h"
#include "mm/init.h"

#include "sched/scheduler.h"

#define QEMU_SERIAL_PHYS 0x9000000

__debug_optimize(3) void arch_early_init() {
#if !defined(AARCH64_USE_16K_PAGES)
    const struct acpi_spcr *const spcr =
        (const struct acpi_spcr *)acpi_lookup_sdt("SPCR");

    uint64_t address = QEMU_SERIAL_PHYS;
    if (spcr != NULL) {
        assert(spcr->interface_kind == ACPI_SPCR_INTERFACE_ARM_PL011);
        assert(spcr->interrupt_kind & __ACPI_SPCR_IRQ_ARM_GIC);
        assert(spcr->serial_port.access_size == ACPI_GAS_ACCESS_SIZE_4_BYTE);
        assert(spcr->baud_rate != ACPI_SPCR_BAUD_RATE_OS_DEPENDENT);

        address = spcr->serial_port.address;
    }

    const uint64_t pte_flags =
        PTE_LEAF_FLAGS | __PTE_INNER_SH | __PTE_MMIO | __PTE_UXN | __PTE_PXN;

    mm_early_identity_map_phys(read_ttbr0_el1(), address, pte_flags);
#endif /* !defined(AARCH64_USE_16K_PAGES) */
}

__debug_optimize(3) void arch_post_mm_init() {
#if !defined(AARCH64_USE_16K_PAGES)
    mm_remove_early_identity_map();
#endif /* !defined(AARCH64_USE_16K_PAGES) */
}

void arch_init_time();
void sched_set_current_thread(struct thread *thread);

__debug_optimize(3) void arch_init_for_smp(struct limine_smp_info *const info) {
    struct smp_boot_info *const boot_info =
        (struct smp_boot_info *)info->extra_argument;

    struct cpu_info *const cpu = boot_info->cpu;

    cpu_init_for_smp(cpu);
    sched_set_current_thread(cpu->idle_thread);

    switch_to_pagemap(&kernel_process.pagemap);
    isr_install_vbar();

    gic_init_on_this_cpu();
    arch_init_time();

    atomic_store_explicit(&boot_info->booted, true, memory_order_seq_cst);

    // On cpu init, we point current-thread to the idle-thread, so we have to
    // call cpu_idle() for the rare case where the scheduler has no threads for
    // us to run, so it returns execution to us.

    sched_yield();
    cpu_idle();

    verify_not_reached();
}

__debug_optimize(3) void arch_init() {
    cpu_init();
    mm_arch_init();
    cpu_post_mm_init();

    isr_install_vbar();
}