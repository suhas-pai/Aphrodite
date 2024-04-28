/*
 * kernel/src/dev/pci/bus.c
 * © suhas pai
 */

#include "cpu/spinlock.h"
#include "mm/kmalloc.h"

#include "bus.h"
#include "resource.h"

static struct array g_root_bus_list = ARRAY_INIT(sizeof(struct pci_bus *));
static struct spinlock g_root_bus_list_lock = SPINLOCK_INIT();

struct pci_bus *
pci_bus_create(struct pci_domain *const domain,
               const uint8_t bus_id,
               const uint8_t segment)
{
    struct pci_bus *const bus = kmalloc(sizeof(*bus));
    if (bus == NULL) {
        return NULL;
    }

    bus->domain = domain;
    bus->resources = ARRAY_INIT(sizeof(struct pci_bus_resource));
    bus->lock = SPINLOCK_INIT();

    bus->bus_id = bus_id;
    bus->segment = segment;

    list_init(&bus->entity_list);
    return bus;
}

__optimize(3) bool pci_add_root_bus(struct pci_bus *const bus) {
    const int flag = spin_acquire_save_irq(&g_root_bus_list_lock);
    const bool result = array_append(&g_root_bus_list, &bus);

    spin_release_restore_irq(&g_root_bus_list_lock, flag);
    return result;
}

__optimize(3) bool pci_remove_root_bus(struct pci_bus *const bus) {
    const int flag = spin_acquire_save_irq(&g_root_bus_list_lock);
    if (!list_empty(&bus->entity_list)) {
        return false;
    }

    uint32_t index = 0;
    array_foreach(&g_root_bus_list, const struct pci_bus *, iter) {
        if (*iter == bus) {
            array_remove_index(&g_root_bus_list, index);
            spin_release_restore_irq(&g_root_bus_list_lock, flag);

            return true;
        }

        index++;
    }

    spin_release_restore_irq(&g_root_bus_list_lock, flag);
    kfree(bus);

    return false;
}

__optimize(3)
const struct array *pci_get_root_bus_list_locked(int *const flag_out) {
    *flag_out = spin_acquire_save_irq(&g_root_bus_list_lock);
    return &g_root_bus_list;
}

__optimize(3) void pci_release_root_bus_list_lock(const int flag) {
    spin_release_restore_irq(&g_root_bus_list_lock, flag);
}