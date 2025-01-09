/*
 * kernel/include/arch/x86_64/asm/pause.h
 * © suhas pai
 */

#pragma once
#define cpu_pause() __builtin_ia32_pause()
