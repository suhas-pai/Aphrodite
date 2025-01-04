/*
 * kernel/src/arch/riscv64/dev/aplic.h
 * © suhas pai
 */

#pragma once

#include "dev/dtb/node.h"
#include "dev/dtb/tree.h"

#include "asm/privl.h"

struct aplic_registers;
struct imsic;
struct aplic {
    struct mmio_region *mmio;
    volatile struct aplic_registers *regs;

    struct imsic *imsic;

    uint32_t gsi_base;
    uint32_t source_count;
    uint32_t msi_parent;
    uint32_t phandle;
};

bool
aplic_init_from_acpi(struct range range,
                     uint16_t source_count,
                     uint32_t gsi_base);
bool
aplic_init_from_dtb(const struct devicetree *tree,
                    const struct devicetree_node *node);

struct aplic *aplic_for_privl(const enum riscv64_privl privl);

bool aplic_enable_irq(enum riscv64_privl privl, uint16_t irq);
bool aplic_disable_irq(enum riscv64_privl privl, uint16_t irq);

void aplic_set_msi_address(enum riscv64_privl privl, uint64_t address);

bool
aplic_set_target_msi(enum riscv64_privl privl,
                     uint32_t irq,
                     uint32_t hart_id,
                     uint32_t guest,
                     uint32_t eiid);
