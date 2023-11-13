/*
 * kernel/src/arch/aarch64/init.c
 * © suhas pai
 */

#include "asm/ttbr.h"
#include "cpu/info.h"

#include "mm/early.h"
#include "mm/init.h"

#include "sys/isr.h"

#define QEMU_SERIAL_PHYS 0x9000000

__optimize(3) void arch_early_init() {
#if !defined(AARCH64_USE_16K_PAGES)
    const uint64_t root_phys = read_ttbr0_el1();
    const uint64_t pte_flags =
        PTE_LEAF_FLAGS | __PTE_INNER_SH | __PTE_MMIO | __PTE_UXN | __PTE_PXN;

    mm_early_identity_map_phys(root_phys, QEMU_SERIAL_PHYS, pte_flags);
#endif /* !defined(AARCH64_USE_16K_PAGES) */
}

__optimize(3) void arch_post_mm_init() {
#if !defined(AARCH64_USE_16K_PAGES)
    mm_remove_early_identity_map();
#endif /* !defined(AARCH64_USE_16K_PAGES) */
}

__optimize(3) void arch_init() {
    cpu_init();
    mm_arch_init();

    isr_install_vbar();
}