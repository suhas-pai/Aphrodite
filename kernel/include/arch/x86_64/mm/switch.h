/*
 * kernel/include/arch/x86_64/mm/types.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

void mm_switch_root(uint64_t root_phys);
