/*
 * kernel/src/arch/riscv64/sys/imsic.h
 * © suhas pai
 */

#pragma once

#include "dev/dtb/node.h"
#include "dev/dtb/tree.h"

#include "asm/privl.h"
#include "sys/isr.h"

void
imsic_init_from_acpi(enum riscv64_privl privl,
                     uint8_t guest_index_bits,
                     uint32_t intr_id_count);

bool
imsic_init_from_dtb(const struct devicetree *tree,
                    const struct devicetree_node *node);

void imsic_enable(enum riscv64_privl privl);

volatile void *imsic_add_region(uint64_t hart_id, struct range range);
volatile void *imsic_region_for_hartid(const uint64_t hart_id);

uint8_t imsic_alloc_msg(enum riscv64_privl privl);
void imsic_free_msg(enum riscv64_privl privl, uint8_t msg);

void
imsic_set_msg_handler(enum riscv64_privl privl, uint8_t msg, isr_func_t func);

void imsic_enable_msg(enum riscv64_privl privl, uint16_t message);
void imsic_disable_msg(enum riscv64_privl privl, uint16_t message);

uint32_t imsic_pop(enum riscv64_privl privl);
void imsic_handle(enum riscv64_privl privl, struct thread_context *context);

void imsic_trigger(enum riscv64_privl privl, uint16_t message);
void imsic_clear(enum riscv64_privl privl, uint16_t message);
