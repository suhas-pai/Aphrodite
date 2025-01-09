/*
 * kernel/include/cpu/isr.h
 * © suhas pai
 */

#pragma once

#include "cpu/info.h"
#include "dev/device.h"
#include "sys/isr.h"

// Returns -1 on alloc failure
void isr_init();

// Terminology:
// - irq: a hardware interrupt number
// - intr: a general interrupt number
// - vector: an index into the interrupt vector table
// - msi[-x]: a message signalled interrupt

isr_vector_t isr_alloc_vector();
isr_vector_t isr_alloc_msi_vector(struct device *device, uint16_t msi_index);

void isr_free_vector(isr_vector_t vector);

void
isr_free_msi_vector(struct device *device,
                    isr_vector_t vector,
                    uint16_t msi_index);

void isr_eoi(uint64_t intr_no);

void
isr_set_vector(isr_vector_t vector,
               isr_func_t handler,
               void *ctx,
               struct arch_isr_info *info);

void
isr_set_msi_vector(isr_vector_t vector,
                   isr_func_t handler,
                   void *ctx,
                   struct arch_isr_info *info);

struct irq_pin *isr_get_irq_pin(uint16_t irq);

bool
isr_install_irq(struct irq_pin *pin,
                isr_func_t handler,
                void *ctx,
                bool masked);

void *isr_uninstall_irq(struct irq_pin *pin);

void isr_mask_irq(struct irq_pin *pin);
void isr_unmask_irq(struct irq_pin *irq);

void isr_mask_intr(isr_vector_t vector);
void isr_unmask_intr(isr_vector_t vector);

uint64_t isr_get_msi_address(const struct cpu_info *cpu, isr_vector_t vector);
uint64_t isr_get_msix_address(const struct cpu_info *cpu, isr_vector_t vector);

enum isr_msi_support : uint8_t {
    ISR_MSI_SUPPORT_NONE,
    ISR_MSI_SUPPORT_MSI,
    ISR_MSI_SUPPORT_MSIX,
};

enum isr_msi_support isr_get_msi_support();
