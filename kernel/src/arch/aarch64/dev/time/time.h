/*
 * kernel/src/arch/aarch64/dev/time/time.h
 * © suhas pai
 */

#pragma once

#include "lib/time.h"
#include "sys/irq.h"

void
enable_gtdt_timer_irqs(uint32_t secure_el1_timer_gsiv,
                       uint32_t non_secure_el1_timer_gsiv,
                       uint32_t virtual_el1_timer_gsiv,
                       uint32_t virtual_el2_timer_gsiv,
                       uint32_t el2_timer_gsiv,
                       enum irq_trigger_mode secure_el1_trigger_mode,
                       enum irq_trigger_mode non_secure_el1_trigger_mode,
                       enum irq_trigger_mode virtual_el1_trigger_mode,
                       enum irq_trigger_mode el2_trigger_mode,
                       enum irq_trigger_mode virtual_el2_trigger_mode);

uint64_t system_timer_get_freq_ns();

nsec_t system_timer_get_count_ns();
nsec_t system_timer_get_compare_ns();
nsec_t system_timer_get_remaining_ns();

void system_timer_oneshot_ns(nsec_t nano);
void system_timer_stop_alarm();
