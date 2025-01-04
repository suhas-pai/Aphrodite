/*
 * kernel/src/sched/irq.h
 * © suhas pai
 */

#pragma once

#include "cpu/info.h"
#include "sys/irq.h"

void sched_init_irq();
void sched_irq_eoi(const irq_number_t irq);

void sched_send_ipi(const struct cpu_info *cpu);
void sched_self_ipi();
