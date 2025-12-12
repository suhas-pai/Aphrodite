/*
 * kernel/include/arch/riscv64/mm/switch.c
 * © suhas pai
 */

#include "asm/csr.h"
#include "asm/satp.h"

#include "mm/mm_types.h"
#include "mm/switch.h"

__debug_optimize(3) void mm_switch_root(const uint64_t root_phys) {
    const uint64_t value =
        (SATP_MODE_39_BIT_PAGING + PAGING_MODE) << SATP_PHYS_MODE_SHIFT |
        (root_phys >> PML1_SHIFT);

    csr_write(satp, value);
    asm volatile ("sfence.vma" ::: "memory");
}
