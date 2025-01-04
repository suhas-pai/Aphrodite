/*
 * kernel/src/dev/pci/bus.h
 * © suhas pai
 */

#pragma once
#include "lib/adt/array.h"

#include "cpu/spinlock.h"
#include "lib/list.h"

#include "domain.h"

struct pci_bus {
    const struct pci_domain *domain;

    struct array resources;
    struct spinlock lock;

    uint8_t bus_id;
    uint8_t segment;

    struct list entity_list;
};

struct pci_bus *
pci_bus_create(struct pci_domain *domain, uint8_t bus_id, uint8_t segment);

bool pci_add_root_bus(struct pci_bus *bus);
bool pci_remove_root_bus(struct pci_bus *bus);

const struct array *pci_get_root_bus_list_locked(int *flag_out);
void pci_release_root_bus_list_lock(int flag);
