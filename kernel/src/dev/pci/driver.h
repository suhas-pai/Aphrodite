/*
 * kernel/dev/pci/driver.h
 * © suhas pai
 */

#pragma once
#include "dev/pci/device.h"

enum pci_driver_match {
    PCI_DRIVER_MATCH_VENDOR_DEVICE,
    PCI_DRIVER_MATCH_VENDOR,

    __PCI_DRIVER_MATCH_CLASS = 1 << 1,
    __PCI_DRIVER_MATCH_SUBCLASS = 1 << 2,
    __PCI_DRIVER_MATCH_PROGIF = 1 << 3,
};

struct pci_driver {
    void (*init)(struct pci_device_info *device);

    uint16_t vendor;
    uint8_t match;

    uint8_t class;
    uint8_t subclass;
    uint8_t prog_if;

    uint8_t device_count;
    uint16_t devices[];
};