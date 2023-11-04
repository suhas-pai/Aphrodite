/*
 * kernel/src/arch/riscv64/init.c
 * © suhas pai
 */

#include "mm/early.h"
#include "mm/init.h"

#define QEMU_SERIAL_PHYS 0x10000000

__optimize(3) void arch_early_init() {
    uint64_t csrr = 0;
    asm volatile ("csrr %0, satp" : "=r" (csrr));

    const uint64_t root_phys = (csrr & ~(0b111ull << 60)) << PML1_SHIFT;
    const uint64_t pte_flags = PTE_LEAF_FLAGS | __PTE_READ | __PTE_WRITE;

    mm_early_identity_map_phys(root_phys, QEMU_SERIAL_PHYS, pte_flags);
}

__optimize(3) void arch_post_mm_init() {
    mm_remove_early_identity_map();
}

__optimize(3) void arch_init() {
    mm_arch_init();
}