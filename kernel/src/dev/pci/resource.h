/*
 * kernel/src/dev/pci/resource.h
 * © suhas pai
 */

#pragma once
#include "mm/mmio.h"

enum pci_bus_resource_kind {
    PCI_BUS_RESOURCE_IO,
    PCI_BUS_RESOURCE_MEM,
    PCI_BUS_RESOURCE_PREFETCH_MEM,
};

struct pci_bus_resource {
    uint64_t host_base;
    uint64_t child_base;
    uint64_t size;

    struct mmio_region *mmio;

    enum pci_bus_resource_kind kind : 2;
    bool is_host_mmio : 1;
};

#define PCI_BUS_RESOURCE_INIT(kind_, \
                              host_base_, \
                              child_base_, \
                              size_, \
                              mmio_, \
                              is_host_mmio_) \
    ((struct pci_bus_resource){ \
        .host_base = (host_base_), \
        .child_base = (child_base_), \
        .size = (size_), \
        .mmio = (mmio_), \
        .kind = (kind_), \
        .is_host_mmio = (is_host_mmio_), \
    })
