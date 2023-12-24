/*
 * kernel/src/dev/pci/drivers/ecam.h
 * © suhas pai
 */

#pragma once

#include "dev/pci/domain.h"
#include "lib/adt/range.h"

#include "lib/list.h"

struct pci_ecam_domain {
    struct pci_domain domain;

    struct list list;
    struct range bus_range;
    struct mmio_region *mmio;
};

struct pci_ecam_domain *
pci_add_ecam_domain(struct range bus_range,
                    uint64_t base_addr,
                    uint16_t segment);

bool pci_remove_ecam_domain(struct pci_ecam_domain *ecam_domain);

uint64_t
pci_ecam_domain_loc_get_offset(const struct pci_ecam_domain *domain,
                               const struct pci_location *loc);

uint8_t
pci_ecam_read_8(const struct pci_ecam_domain *domain,
                const struct pci_location *loc,
                uint16_t offset);

uint16_t
pci_ecam_read_16(const struct pci_ecam_domain *domain,
                 const struct pci_location *loc,
                 uint16_t offset);

uint32_t
pci_ecam_read_32(const struct pci_ecam_domain *domain,
                 const struct pci_location *loc,
                 uint16_t offset);

uint64_t
pci_ecam_read_64(const struct pci_ecam_domain *domain,
                 const struct pci_location *loc,
                 uint16_t offset);

void
pci_ecam_write_8(const struct pci_ecam_domain *domain,
                 const struct pci_location *loc,
                 uint16_t offset,
                 uint8_t value);

void
pci_ecam_write_16(const struct pci_ecam_domain *domain,
                  const struct pci_location *loc,
                  uint16_t offset,
                  uint16_t value);

void
pci_ecam_write_32(const struct pci_ecam_domain *domain,
                  const struct pci_location *loc,
                  uint16_t offset,
                  uint32_t value);

void
pci_ecam_write_64(const struct pci_ecam_domain *domain,
                  const struct pci_location *loc,
                  uint16_t offset,
                  uint64_t value);

