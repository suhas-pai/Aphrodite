/*
 * kernel/include/arch/x86_64/cpu/init.h
 * © suhas pai
 */

#pragma once

void cpu_init();
void cpu_early_init();
void cpu_init_for_smp();

void cpu_post_mm_init();
