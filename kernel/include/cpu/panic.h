/*
 * kernel/include/cpu/panic.h
 * © suhas pai
 */

#pragma once
#include "lib/macros.h"

__printf_format(1, 2) [[noreturn]] void panic(const char *fmt, ...);
