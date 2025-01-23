/*
 * kernel/src/dev/pci/driver.c
 * © suhas pai
 */

#include "dev/pci/device.h"

#include "dev/bus.h"
#include "dev/init.h"

static struct pci_device g_pci_device = {
    .bus = BUS_INIT(g_pci_device.bus, /*parent=*/nullptr, (bus_probe_t)no_op),
    .entity_list = LIST_INIT(g_pci_device.entity_list),
};

__debug_optimize(3) struct pci_device *pci_device() {
    return &g_pci_device;
}

static void init_pci_device() {
    bus_init_root(&g_pci_device.bus);
}

MAKE_DEV_INIT_FUNC(init_pci_device);
