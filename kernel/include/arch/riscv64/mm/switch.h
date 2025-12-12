/*
 * kernel/include/arch/riscv64/mm/switch.h
 * © suhas pai
 */

#include <stdint.h>

void mm_switch_root(uint64_t root_phys);
