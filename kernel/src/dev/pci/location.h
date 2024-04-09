/*
 * kernel/src/dev/pci/location.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

struct pci_location {
    uint8_t segment;
    uint8_t bus;
    uint8_t slot;
    uint8_t function;
};

#define PCI_LOCATION_NULL() \
    ((struct pci_location){ \
        .segment = 0, \
        .bus = 0, \
        .slot = 0, \
        .function = 0 \
    })
