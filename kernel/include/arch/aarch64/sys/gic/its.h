/*
 * kernel/src/arch/aarch64/sys/gic/its.h
 * © suhas pai
 */

#pragma once

#include "dev/dtb/node.h"
#include "dev/dtb/tree.h"

#include "cpu/spinlock.h"
#include "dev/device.h"

#include "sys/isr.h"

#define GIC_ITS_LPI_INTERRUPT_START 8192
#define GIC_ITS_LPI_INTERRUPT_LAST 65535

#define GIC_ITS_MAX_LPIS_SUPPORTED 8192

struct gic_its_info {
    struct list list;

    struct spinlock bitset_lock;
    struct spinlock queue_lock;

    uint32_t id;
    uint64_t phys_addr;

    uint16_t queue_free_slot_count;
    uint16_t device_table_entry_size;
    uint16_t int_collection_table_entry_count;

    struct mmio_region *mmio;
    uint64_t *bitset;

    void *device_table;
    void *int_collection_table;
};

struct gic_its_info *
gic_its_init_from_dtb(const struct devicetree *tree,
                      const struct devicetree_node *node);

struct gic_its_info *gic_its_init_from_info(uint32_t id, uint64_t phys_addr);
struct list *gic_its_get_list();

isr_vector_t
gic_its_alloc_msi_vector(struct gic_its_info *its,
                         struct device *device,
                         const uint16_t msi_index);

void
gic_its_free_msi_vector(struct gic_its_info *its,
                        struct device *device,
                        isr_vector_t vector,
                        uint16_t msi_index);

volatile uint64_t *gic_its_get_msi_address(struct gic_its_info *its);
