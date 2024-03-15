/*
 * kernel/src/cpu/isr.h
 * © suhas pai
 */

#pragma once
#include <stdbool.h>

#include "cpu/info.h"
#include "sys/isr.h"

// Returns -1 on alloc failure
void isr_init();

isr_vector_t isr_alloc_vector(bool for_msi);
void isr_free_vector(isr_vector_t vector, bool for_msi);

void isr_eoi(uint64_t intr_no);

void
isr_set_vector(isr_vector_t vector,
               isr_func_t handler,
               struct arch_isr_info *info);

void
isr_assign_irq_to_cpu(struct cpu_info *cpu,
                      uint8_t irq,
                      isr_vector_t vector,
                      bool masked);

void isr_mask_irq(isr_vector_t irq);
void isr_unmask_irq(isr_vector_t irq);

uint64_t isr_get_msi_address(const struct cpu_info *cpu, isr_vector_t vector);
uint64_t isr_get_msix_address(const struct cpu_info *cpu, isr_vector_t vector);
