/*
 * kernel/src/dev/pci/bar.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/range.h"
#include "mm/mmio.h"

struct pci_entity_bar_info {
    struct range port_or_phys_range;
    struct mmio_region *mmio;

    uint32_t index_in_mmio;

    bool is_present : 1;
    bool is_mmio : 1;
    bool is_prefetchable : 1;
    bool is_64_bit : 1;
};

bool pci_map_bar(struct pci_entity_bar_info *bar);
bool pci_unmap_bar(struct pci_entity_bar_info *bar);

struct pci_entity_info;

uint8_t
pci_entity_bar_read8(struct pci_entity_info *entity,
                     struct pci_entity_bar_info *const bar,
                     uint32_t offset);

uint16_t
pci_entity_bar_read16(struct pci_entity_info *entity,
                      struct pci_entity_bar_info *const bar,
                      uint32_t offset);

uint32_t
pci_entity_bar_read32(struct pci_entity_info *entity,
                      struct pci_entity_bar_info *const bar,
                      uint32_t offset);

uint64_t
pci_entity_bar_read64(struct pci_entity_info *entity,
                      struct pci_entity_bar_info *const bar,
                      uint32_t offset);

void
pci_entity_bar_write8(struct pci_entity_info *entity,
                      struct pci_entity_bar_info *const bar,
                      uint32_t offset,
                      uint8_t value);

void
pci_entity_bar_write16(struct pci_entity_info *entity,
                       struct pci_entity_bar_info *const bar,
                       uint32_t offset,
                       uint16_t value);

void
pci_entity_bar_write32(struct pci_entity_info *entity,
                       struct pci_entity_bar_info *const bar,
                       uint32_t offset,
                       uint32_t value);

void
pci_entity_bar_write64(struct pci_entity_info *entity,
                       struct pci_entity_bar_info *const bar,
                       uint32_t offset,
                       uint64_t value);
