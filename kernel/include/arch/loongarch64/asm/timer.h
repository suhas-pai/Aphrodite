/*
 * kernel/include/arch/loongarch64/asm/timer.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

enum timer_config_shift : uint8_t {
    TIMER_CONFIG_INIT_VALUE_SHIFT = 2
};

enum timer_config : uint8_t {
    __TIMER_CONFIG_ENABLE = 1 << 0,
    __TIMER_CONFIG_PERIODIC = 1 << 1
};

enum timer_interrupt : uint8_t {
    __TIMER_INTR_CLEAR = 1 << 0
};
