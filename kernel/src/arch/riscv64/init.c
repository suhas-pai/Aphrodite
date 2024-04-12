/*
 * kernel/src/arch/riscv64/init.c
 * © suhas pai
 */

#include "asm/irqs.h"
#include "asm/satp.h"

#include "cpu/init.h"

#include "mm/early.h"
#include "mm/init.h"

#define QEMU_SERIAL_PHYS 0x10000000

__optimize(3) void arch_early_init() {
    cpu_init();
    disable_interrupts();

    const uint64_t pte_flags = PTE_LEAF_FLAGS | __PTE_READ | __PTE_WRITE;
    const uint64_t root_phys =
         (read_satp() & __SATP_PHYS_ROOT_NUM) << PML1_SHIFT;

    mm_early_identity_map_phys(root_phys, QEMU_SERIAL_PHYS, pte_flags);
}

__optimize(3) void arch_post_mm_init() {
    mm_remove_early_identity_map();
}

__optimize(3) void arch_init() {
    mm_arch_init();
}