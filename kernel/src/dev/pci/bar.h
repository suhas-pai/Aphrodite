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

volatile void *pci_entity_bar_get_base(const struct pci_entity_bar_info *bar);
struct pci_entity_info;

uint8_t
pci_bar_read_u8(struct pci_entity_info *entity,
                struct pci_entity_bar_info *bar,
                uint32_t offset);

uint16_t
pci_bar_read_u16(struct pci_entity_info *entity,
                 struct pci_entity_bar_info *bar,
                 uint32_t offset);

uint32_t
pci_bar_read_u32(struct pci_entity_info *entity,
                 struct pci_entity_bar_info *bar,
                 uint32_t offset);

uint64_t
pci_bar_read_u64(struct pci_entity_info *entity,
                 struct pci_entity_bar_info *bar,
                 uint32_t offset);

void
pci_bar_write_u8(struct pci_entity_info *entity,
                 struct pci_entity_bar_info *bar,
                 uint32_t offset,
                 uint8_t value);

void
pci_bar_write_u16(struct pci_entity_info *entity,
                  struct pci_entity_bar_info *bar,
                  uint32_t offset,
                  uint16_t value);

void
pci_bar_write_u32(struct pci_entity_info *entity,
                  struct pci_entity_bar_info *bar,
                  uint32_t offset,
                  uint32_t value);

void
pci_bar_write_u64(struct pci_entity_info *entity,
                  struct pci_entity_bar_info *bar,
                  uint32_t offset,
                  uint64_t value);
