/*
 * kernel/include/dev/pci/bus.h
 * © suhas pai
 */

#pragma once
#include <lib/adt/array.h>

#include "dev/pci/domain.h"
#include "dev/bus.h"

struct pci_bus {
    struct bus bus;
    struct array resources;

    uint8_t bus_id;
    uint8_t segment;

    struct list entity_list;
};

#define pci_bus_foreach_entity(bus, entity) \
    struct pci_entity *entity = nullptr; \
    list_foreach(&bus->entity_list, list_in_bus, entity)

struct pci_bus *
pci_bus_create(struct pci_domain *domain,
               uint8_t bus_id,
               uint8_t segment);

struct pci_domain *pci_bus_get_domain(struct pci_bus *bus);
