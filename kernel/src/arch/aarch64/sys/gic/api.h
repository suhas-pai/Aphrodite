/*
 * kernel/src/arch/aarch64/sys/gic/api.h
 * © suhas pai
 */

#pragma once

#include "cpu/info.h"
#include "dev/device.h"

#include "sys/irq.h"
#include "sys/isr.h"

#define GIC_SGI_INTERRUPT_START 0
#define GIC_SGI_INTERRUPT_LAST 15

#define GIC_SGI_IRQ_RANGE \
    RANGE_INIT(GIC_SGI_INTERRUPT_START, GIC_SGI_INTERRUPT_LAST)

#define GIC_PPI_INTERRUPT_START 16
#define GIC_PPI_INTERRUPT_LAST 31

#define GIC_PPI_IRQ_RANGE \
    RANGE_INIT(GIC_PPI_INTERRUPT_START, GIC_PPI_INTERRUPT_LAST)

#define GIC_SPI_INTERRUPT_START 32
#define GIC_SPI_INTERRUPT_LAST 1020

#define IRQ_NUMBER_FMT "%" PRIu32

void gic_init_from_dtb();
void gic_init_on_this_cpu(uint64_t phys_addr, uint64_t size);

void gic_set_version(uint8_t gic);

void gicd_mask_irq(irq_number_t irq);
void gicd_unmask_irq(irq_number_t irq);

isr_vector_t
gicd_alloc_msi_vector(struct device *device, const uint16_t msi_index);

void gicd_free_msi_vector(isr_vector_t vector, uint16_t msi_index);

void gicd_set_irq_affinity(irq_number_t irq, uint8_t iface);
void gicd_set_irq_trigger_mode(irq_number_t irq, enum irq_trigger_mpde mode);
void gicd_set_irq_priority(irq_number_t irq, uint8_t priority);

void gicd_send_ipi(const struct cpu_info *cpu, uint8_t int_no);
void gicd_send_sipi(uint8_t int_no);

volatile uint64_t *gicd_get_msi_address(isr_vector_t vector);
enum isr_msi_support gicd_get_msi_support();

irq_number_t gic_cpu_get_irq_number(uint8_t *cpu_id_out);
uint32_t gic_cpu_get_irq_priority();

void gic_cpu_eoi(uint8_t cpu_id, irq_number_t irq_number);
