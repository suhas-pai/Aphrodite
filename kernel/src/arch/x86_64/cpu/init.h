/*
 * kernel/src/arch/x86_64/cpu/init.h
 * © suhas pai
 */

#pragma once
#include "limine.h"

void cpu_init();
void cpu_early_init();
void cpu_init_for_smp(const struct limine_smp_info *cpu);
