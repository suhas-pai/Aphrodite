/*
 * kernel/dev/pci/pci.h
 * © suhas pai
 */

#pragma once
#include "device.h"

struct pci_domain *
pci_add_pcie_domain(struct range bus_range,
                    uint64_t base_addr,
                    uint16_t segment);

void pci_init();