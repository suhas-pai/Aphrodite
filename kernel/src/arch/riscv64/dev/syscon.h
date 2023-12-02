/*
 * kernel/src/arch/riscv64/dev/syscon.h
 * © suhas pai
 */

#pragma once
#include "lib/macros.h"

__noreturn void syscon_poweroff();
__noreturn void syscon_reboot();