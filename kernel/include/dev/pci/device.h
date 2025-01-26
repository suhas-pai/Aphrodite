/*
 * kernel/include/dev/pci/device.h
 * © suhas pai
 */

#pragma once
#include "dev/bus.h"

struct pci_device {
    struct bus bus;
    struct list entity_list;
};

#define pci_device_foreach_driver(iter) \
    bus_foreach_driver(&pci_device()->bus, struct pci_driver, driver.list, iter)

#define pci_device_foreach_entity(entity) \
    struct pci_entity *entity = nullptr; \
    list_foreach(entity, &pci_device()->entity_list, list_in_device)

struct pci_device *pci_device();
