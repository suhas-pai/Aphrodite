/*
 * kernel/include/arch/riscv64/mm/switch.c
 * © suhas pai
 */

#include "asm/csr.h"
#include "mm/switch.h"

__debug_optimize(3) void
mm_switch_root(const uint64_t lower_root_phys, const uint64_t higher_root_phys)
{
    csr_write(pgdl, lower_root_phys);
    csr_write(pgdh, higher_root_phys);
}
