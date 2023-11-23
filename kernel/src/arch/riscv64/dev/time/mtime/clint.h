/*
 * kernel/src/arch/riscv64/dev/time/mtime/clint.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

void clint_init(volatile uint64_t *base, uint64_t size, uint64_t freq);