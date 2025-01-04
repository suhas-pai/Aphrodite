/*
 * kernel/src/cpu/util.h
 * © suhas pai
 */

#pragma once
#include "lib/macros.h"

__noreturn void cpu_idle();
__noreturn void cpu_halt();

__noreturn void cpu_shutdown();
__noreturn void cpu_reboot();
