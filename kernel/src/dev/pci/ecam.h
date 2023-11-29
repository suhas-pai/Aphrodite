/*
 * kernel/src/dev/pci/drivers/ecam.h
 * © suhas pai
 */

#pragma once

#include "dev/pci/space.h"
#include "lib/adt/range.h"

#include "lib/list.h"

struct pci_ecam_space {
    struct pci_space space;

    struct list list;
    struct range bus_range;
    struct mmio_region *mmio;
};

struct pci_ecam_space *
pci_add_ecam_space(struct range bus_range,
                   uint64_t base_addr,
                   uint16_t segment);

bool pci_remove_ecam_space(struct pci_ecam_space *ecam_space);

uint64_t
pci_ecam_space_loc_get_offset(const struct pci_ecam_space *space,
                              const struct pci_space_location *loc);

uint8_t
pci_ecam_read_8(struct pci_ecam_space *space,
                const struct pci_space_location *loc,
                uint16_t offset);

uint16_t
pci_ecam_read_16(struct pci_ecam_space *space,
                 const struct pci_space_location *loc,
                 uint16_t offset);

uint32_t
pci_ecam_read_32(struct pci_ecam_space *space,
                 const struct pci_space_location *loc,
                 uint16_t offset);

uint64_t
pci_ecam_read_64(struct pci_ecam_space *space,
                 const struct pci_space_location *loc,
                 uint16_t offset);

void
pci_ecam_write_8(struct pci_ecam_space *space,
                 const struct pci_space_location *loc,
                 uint16_t offset,
                 uint8_t value);

void
pci_ecam_write_16(struct pci_ecam_space *space,
                  const struct pci_space_location *loc,
                  uint16_t offset,
                  uint16_t value);

void
pci_ecam_write_32(struct pci_ecam_space *space,
                  const struct pci_space_location *loc,
                  uint16_t offset,
                  uint32_t value);

void
pci_ecam_write_64(struct pci_ecam_space *space,
                  const struct pci_space_location *loc,
                  uint16_t offset,
                  uint64_t value);

