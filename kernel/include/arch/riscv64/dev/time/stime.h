/*
 * kernel/src/arch/riscv64/dev/time/stime.h
 * © suhas pai
 */

#pragma once
#include "lib/time.h"

usec_t stime_get();

void stimer_oneshot(const usec_t interval);
void stimer_stop();
