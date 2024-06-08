/*
 * kernel/src/arch/aarch64/sys/gic/v2.h
 * © suhas pai
 */

#pragma once

#include "dev/dtb/node.h"
#include "dev/dtb/tree.h"

#include "cpu/isr.h"
#include "sys/isr.h"

struct cpu_info;

bool
gicv2_init_from_dtb(const struct devicetree *tree,
                    const struct devicetree_node *node);

void gicv2_init_on_this_cpu(struct range range);
bool gicv2_init_from_info(uint64_t phys_base_address);

isr_vector_t gicdv2_alloc_msi_vector();
void gicdv2_free_msi_vector(isr_vector_t vector);

void gicv2_add_msi_frame(uint64_t phys_base_address);

#define IRQ_NUMBER_FMT "%" PRIu32

void gicdv2_mask_irq(irq_number_t irq);
void gicdv2_unmask_irq(irq_number_t irq);

void gicdv2_set_irq_affinity(irq_number_t irq, uint8_t iface);
void gicdv2_set_irq_trigger_mode(irq_number_t irq, enum irq_trigger_mode mode);
void gicdv2_set_irq_priority(irq_number_t irq, uint8_t priority);

void gicdv2_send_ipi(const struct cpu_info *cpu, uint8_t int_no);
void gicdv2_send_sipi(uint8_t int_no);

volatile uint64_t *gicdv2_get_msi_address(isr_vector_t vector);
enum isr_msi_support gicdv2_get_msi_support();

irq_number_t gicv2_cpu_get_irq_number(uint8_t *cpu_id_out);
uint32_t gicv2_cpu_get_irq_priority();

void gicv2_cpu_eoi(uint8_t cpu_id, irq_number_t irq_number);
