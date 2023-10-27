/*
 * kernel/arch/x86_64/apic/init.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

void apic_init(uint64_t lapic_regs_base);