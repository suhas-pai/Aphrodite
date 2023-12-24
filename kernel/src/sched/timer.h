/*
 * kernel/src/sched/timer.h
 * © suhas pai
 */

#pragma once
#include "lib/time.h"

void sched_timer_oneshot(usec_t usec);
void sched_irq_eoi();