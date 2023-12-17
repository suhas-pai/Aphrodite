/*
 * kernel/src/dev/pci/location.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

struct pci_location {
    uint32_t segment;
    uint32_t bus;
    uint32_t slot;
    uint32_t function;
};
