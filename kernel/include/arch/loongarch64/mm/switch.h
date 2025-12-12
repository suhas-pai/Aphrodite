/*
 * kernel/include/arch/riscv64/mm/switch.h
 * © suhas pai
 */

#include <stdint.h>

void mm_switch_root(uint64_t lower_root_phys, uint64_t higher_root_phys);
