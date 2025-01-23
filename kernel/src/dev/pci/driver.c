/*
 * kernel/src/dev/pci/driver.c
 * © suhas pai
 */

#include "dev/pci/device.h"
#include "dev/pci/driver.h"

void pci_register_driver(struct pci_driver *const pci_driver) {
    bus_add_driver(&pci_device()->bus, &pci_driver->driver);
}

void pci_unregister_driver(struct pci_driver *const pci_driver) {
    bus_remove_driver(&pci_device()->bus, &pci_driver->driver);
}
