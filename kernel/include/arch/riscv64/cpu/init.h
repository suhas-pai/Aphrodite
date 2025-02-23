/*
 * kernel/include/arch/riscv64/cpu/init.h
 * © suhas pai
 */

#pragma once

void cpu_init();
void cpu_early_init();
void cpu_init_from_dtb();

void cpu_post_mm_init();
