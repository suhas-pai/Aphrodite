/*
 * kernel/src/arch/x86_64/dev/ide/init.c
 * © suhas pai
 */

#pragma once
#include <stdint.h>

void
ide_init(uint32_t bar0,
         uint32_t bar1,
         uint32_t bar2,
         uint32_t bar3,
         uint32_t bar4);
