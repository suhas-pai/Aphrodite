/*
 * kernel/src/arch/aarch64/dev/time/time.h
 * © suhas pai
 */

#pragma once
#include "lib/time.h"

uint64_t system_timer_get_frequency_ns();

nsec_t system_timer_get_count_ns();
nsec_t system_timer_get_compare_ns();
nsec_t system_timer_get_remaining_ns();

void system_timer_oneshot_ns(nsec_t nano);
void system_timer_stop_alarm();
