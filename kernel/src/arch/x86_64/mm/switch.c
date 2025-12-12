/*
 * kernel/src/arch/x86_64/mm/switch.c
 * © suhas pai
 */

#include "asm/regs.h"
#include "mm/switch.h"

__debug_optimize(3) void mm_switch_root(const uint64_t root_phys) {
    cr3_write(root_phys);
}
