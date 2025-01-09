/*
 * kernel/include/arch/aarch64/cpu/init.h
 * © suhas pai
 */

#pragma once
#include "cpu/info.h"

void cpu_init();
void cpu_early_init();
void cpu_post_mm_init();

void cpu_init_for_smp(struct cpu_info *cpu);
