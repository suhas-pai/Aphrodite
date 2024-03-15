/*
 * kernel/src/cpu/panic.h
 * © suhas pai
 */

#pragma once
#include "lib/macros.h"

__printf_format(1, 2) __noreturn void panic(const char *fmt, ...);
