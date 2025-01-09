/*
 * kernel/include/arch/x86_64/sys/gdt.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

void gdt_load();

uint16_t gdt_get_kernel_code_segment();
uint16_t gdt_get_user_data_segment();
