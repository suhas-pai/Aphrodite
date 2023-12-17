/*
 * kernel/src/arch/x86_64/dev/pci/legacy.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include "dev/pci/location.h"

uint64_t
pci_legacy_domain_read(const struct pci_location *loc,
                       uint32_t offset,
                       uint8_t access_size);

bool
pci_legacy_domain_write(const struct pci_location *loc,
                        uint32_t offset,
                        uint32_t value,
                        uint8_t access_size);