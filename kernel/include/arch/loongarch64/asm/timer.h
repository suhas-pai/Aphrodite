/*
 * kernel/src/arch/loongarch64/asm/timer.h
 * © suhas pai
 */

#pragma once

enum timer_config_shift {
    TIMER_CONFIG_INIT_VALUE_SHIFT = 2
};

enum timer_config {
    __TIMER_CONFIG_ENABLE = 1 << 0,
    __TIMER_CONFIG_PERIODIC = 1 << 1
};

enum timer_interrupt {
    __TIMER_INTR_CLEAR = 1 << 0
};
