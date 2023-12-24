/*
 * kernel/src/dev/pci/bus.h
 * © suhas pai
 */

#pragma once

#include "lib/list.h"
#include "domain.h"

struct pci_bus {
    const struct pci_domain *domain;
    struct array resources;

    uint32_t bus_id;
    uint32_t segment;

    struct list entity_list;
};

struct pci_bus *
pci_bus_create(struct pci_domain *domain, uint32_t bus_id, uint32_t segment);

bool pci_add_root_bus(struct pci_bus *bus);
bool pci_remove_root_bus(struct pci_bus *bus);

const struct array *pci_get_root_bus_list_locked(int *flag_out);
void pci_release_root_bus_list_lock(int flag);