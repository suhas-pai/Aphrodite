/*
 * kernel/src/sched/irq.h
 * © suhas pai
 */

#pragma once

#include "cpu/info.h"
#include "sys/isr.h"

void sched_init_irq();
void sched_irq_eoi();

void sched_send_ipi(const struct cpu_info *cpu);
void sched_self_ipi();

isr_vector_t sched_get_isr_vector();
