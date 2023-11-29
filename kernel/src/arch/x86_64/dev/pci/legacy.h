/*
 * kernel/src/arch/x86_64/dev/pci/legacy.h
 * © suhas pai
 */

#pragma once
#include "dev/pci/space.h"

uint64_t
pci_legacy_space_read(const struct pci_space_location *loc,
                      uint32_t offset,
                      uint8_t access_size);

bool
pci_legacy_space_write(const struct pci_space_location *loc,
                       uint32_t offset,
                       uint32_t value,
                       uint8_t access_size);