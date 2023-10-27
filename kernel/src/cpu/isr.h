/*
 * kernel/cpu/isr.h
 * © suhas pai
 */

#pragma once
#include <stdbool.h>

#include "asm/irq_context.h"
#include "cpu/info.h"
#include "sys/isr.h"

// Returns -1 on alloc failure
void isr_init();
isr_vector_t isr_alloc_vector();

void
isr_set_vector(isr_vector_t vector,
               isr_func_t handler,
               struct arch_isr_info *info);

void
isr_assign_irq_to_cpu(struct cpu_info *cpu,
                      uint8_t irq,
                      isr_vector_t vector,
                      bool masked);