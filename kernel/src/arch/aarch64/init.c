/*
 * kernel/src/arch/aarch64/init.c
 * © suhas pai
 */

#include "acpi/api.h"

#include "asm/ttbr.h"
#include "cpu/init.h"

#include "mm/early.h"
#include "mm/init.h"

#include "sys/isr.h"

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

__debug_optimize(3) void arch_init() {
    cpu_init();
    mm_arch_init();

    isr_install_vbar();
}