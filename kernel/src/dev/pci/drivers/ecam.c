/*
 * kernel/src/dev/pci/drivers/ecam.c
 * © suhas pai
 */

#include "dev/driver.h"
#include "dev/printk.h"

#include "../pci.h"

static bool
init_from_dtb(const struct devicetree *const tree,
              const struct devicetree_node *const node)
{
    (void)tree;
    const struct devicetree_prop_reg *const reg_prop =
        (const struct devicetree_prop_reg *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_REG);

    if (reg_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "pci-ecam: 'reg' property in dtb node is missing\n");
        return false;
    }

    if (array_empty(reg_prop->list)) {
        printk(LOGLEVEL_WARN,
               "pci-ecam: 'reg' property in dtb node is empty\n");
        return false;
    }

    const struct devicetree_prop_bus_range *const bus_range_prop =
        (const struct devicetree_prop_bus_range *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_PCI_BUS_RANGE);

    if (bus_range_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "pci-ecam: 'bus-range' property in dtb node is missing\n");
        return false;
    }

    const struct range bus_range = bus_range_prop->range;
    uint64_t bus_range_end = 0;

    if (!range_get_end(bus_range, &bus_range_end)) {
        printk(LOGLEVEL_WARN,
               "pci-ecam: 'bus-range' provided in dtb node overflows\n");
        return false;
    }

    if (bus_range_end > UINT8_MAX) {
        printk(LOGLEVEL_WARN,
               "pci-ecam: 'bus-range' provided in dtb node " RANGE_FMT "is too "
               "wide\n",
               RANGE_FMT_ARGS(bus_range));
        return false;
    }

    const struct devicetree_prop_reg_info *const mmio_reg =
        (const struct devicetree_prop_reg_info *)array_front(reg_prop->list);

    const struct range mmio_range =
        RANGE_INIT(mmio_reg->address, mmio_reg->size);

    if (!range_has_index_range(mmio_range,
                               RANGE_INIT(bus_range.front,
                                          bus_range.size << 20)))
    {
        printk(LOGLEVEL_INFO,
               "pci-ecam: bus-range " RANGE_FMT " of dtb node can't fit in "
               "provided mmio-range " RANGE_FMT "\n",
               RANGE_FMT_ARGS(bus_range),
               RANGE_FMT_ARGS(mmio_range));
        return false;
    }

    pci_add_pcie_domain(bus_range, mmio_range.front, /*segment=*/0);
    return true;
}

static const char *const compat_list[] = { "pci-host-ecam-generic" };
static const struct dtb_driver dtb_driver = {
    .init = init_from_dtb,
    .match_flags = __DTB_DRIVER_MATCH_COMPAT,

    .compat_list = compat_list,
    .compat_count = countof(compat_list),
};

__driver static const struct driver driver = {
    .name = "pci-ecam-driver",
    .dtb = &dtb_driver,
    .pci = NULL
};