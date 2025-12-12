/*
 * kernel/src/arch/aarch64/mm/switch.c
 * © suhas pai
 */

#include "asm/ttbr.h"
#include "mm/switch.h"

__debug_optimize(3) void
mm_switch_root(const uint64_t lower_root_phys, const uint64_t higher_root_phys)
{
    ttbr0_el1_write(lower_root_phys);
    ttbr1_el1_write(higher_root_phys);

#if defined(AARCH64_CONFIG_16K_PAGES)
    tcr_el1_write(rm_mask(tcr_el1_read(), __TCR_TG1)
                | TCR_TG1_16KIB << TCR_TG1_SHIFT);
#endif /* defined(AARCH64_CONFIG_16K_PAGES) */

    asm volatile ("dsb sy; isb" ::: "memory");
}
