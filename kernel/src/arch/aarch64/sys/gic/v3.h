/*
 * kernel/src/arch/aarch64/sys/gic/v3.h
 * © suhas pai
 */

#pragma once

#include "dev/dtb/node.h"
#include "dev/dtb/tree.h"

#include "cpu/info.h"
#include "dev/device.h"
#include "sys/isr.h"

#define IRQ_NUMBER_FMT "%" PRIu32

bool
gicv3_init_from_dtb(const struct devicetree *tree,
                    const struct devicetree_node *node);

void gicv3_init_from_info(uint64_t dist_phys_addr, struct range redist_range);
void gicv3_init_on_this_cpu();

void gicdv3_mask_irq(irq_number_t irq);
void gicdv3_unmask_irq(irq_number_t irq);

isr_vector_t
gicdv3_alloc_msi_vector(struct device *device, const uint16_t msi_index);

void gicdv3_free_msi_vector(isr_vector_t vector, uint16_t msi_index);

void gicdv3_set_irq_affinity(irq_number_t irq, uint8_t iface);
void gicdv3_set_irq_trigger_mode(irq_number_t irq, enum irq_trigger_mpde mode);
void gicdv3_set_irq_priority(irq_number_t irq, uint8_t priority);

void gicdv3_send_ipi(const struct cpu_info *cpu, uint8_t int_no);
void gicdv3_send_sipi(uint8_t int_no);

volatile uint64_t *gicdv3_get_msi_address(isr_vector_t vector);
enum isr_msi_support gicdv3_get_msi_support();

irq_number_t gicv3_cpu_get_irq_number(uint8_t *cpu_id_out);
uint32_t gicv3_cpu_get_irq_priority();

void gicv3_cpu_eoi(uint8_t cpu_id, irq_number_t irq_number);
