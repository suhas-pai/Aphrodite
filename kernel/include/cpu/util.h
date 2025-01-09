/*
 * kernel/include/cpu/util.h
 * © suhas pai
 */

#pragma once
#include "lib/macros.h"

[[noreturn]] void cpu_idle();
[[noreturn]] void cpu_halt();

[[noreturn]] void cpu_shutdown();
[[noreturn]] void cpu_reboot();
