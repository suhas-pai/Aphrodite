/*
 * kernel/src/arch/loongarch64/sys/time.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

void timer_oneshot(uint32_t seconds);
void timer_stop();

uint32_t timer_remaining();
