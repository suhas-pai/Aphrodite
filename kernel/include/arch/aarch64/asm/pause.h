/*
 * kernel/include/arch/aarch64/asm/pause.h
 * © suhas pai
 */

#pragma once

#define cpu_pause() asm("yield")
