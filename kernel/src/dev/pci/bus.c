/*
 * kernel/src/dev/pci/bus.c
 * © suhas pai
 */

#include "dev/pci/bus.h"
#include "dev/pci/probe.h"
#include "dev/pci/resource.h"

#include "mm/kmalloc.h"

struct pci_bus *
pci_bus_create(struct pci_domain *const domain,
               const uint8_t bus_id,
               const uint8_t segment)
{
    struct pci_bus *const pci_bus = kmalloc(sizeof(*pci_bus));
    if (pci_bus == nullptr) {
        return nullptr;
    }

    list_init(&pci_bus->entity_list);
    bus_init(&pci_bus->bus,
             /*parent=*/&domain->bus,
             SV_STATIC("pci-bus"),
             /*driver=*/nullptr,
             pci_bus_probe);

    pci_bus->resources = ARRAY_INIT(sizeof(struct pci_bus_resource));

    pci_bus->bus_id = bus_id;
    pci_bus->segment = segment;

    return pci_bus;
}

struct pci_domain *pci_bus_get_domain(struct pci_bus *const pci_bus) {
    return parent_of(bus_get_dev_parent(&pci_bus->bus), struct pci_domain, bus);
}
