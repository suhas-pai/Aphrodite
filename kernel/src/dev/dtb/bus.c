/*
 * kernel/src/dev/dtb/bus.c
 * © suhas pai
 */

#include "dev/dtb/bus.h"
#include "dev/dtb/device.h"
#include "dev/dtb/driver.h"

#include "dev/init.h"
#include "mm/kmalloc.h"

static struct simple_alloc g_alloc;

bool
dtb_init_nodes_for_driver(struct dtb_driver *const dtb_driver,
                          const struct devicetree *const tree,
                          const struct devicetree_node *const node)
{
    bool result = false;
    if (dtb_driver->match_flags & __DTB_DRIVER_MATCH_COMPAT) {
        struct devicetree_prop_compat *const compat_prop =
            (struct devicetree_prop_compat *)(uint64_t)
                devicetree_node_get_prop(node, DEVICETREE_PROP_COMPAT);

        if (compat_prop == nullptr) {
            goto next;
        }

        bool found = false;
        arrptr_foreach(dtb_driver->compat_list,
                       dtb_driver->compat_count,
                       compat_list)
        {
            if (devicetree_prop_compat_has_sv(compat_prop, *compat_list)) {
                found = true;
                break;
            }
        }

        if (!found) {
            goto next;
        }
    }

    if (dtb_driver->match_flags & __DTB_DRIVER_MATCH_DEVICE_TYPE) {
        struct devicetree_prop_device_type *const device_type_prop =
            (struct devicetree_prop_device_type *)(uint64_t)
                devicetree_node_get_prop(node, DEVICETREE_PROP_DEVICE_TYPE);

        if (device_type_prop == nullptr) {
            goto next;
        }

        if (!sv_equals(device_type_prop->name, dtb_driver->device_type)) {
            goto next;
        }
    }

    struct dtb_device *const device = kmalloc(sizeof(struct dtb_device));
    if (device == nullptr) {
        return false;
    }

    device_initialize(&device->device,
                      &dtb_bus()->bus,
                      /*driver=*/&dtb_driver->driver,
                      /*init_name=*/SV_EMPTY());

    device->tree = tree;
    device->node = node;

    result = device_probe(&device->device);

next:
    devicetree_node_foreach_child(node, iter) {
        if (dtb_init_nodes_for_driver(dtb_driver, tree, iter)) {
            result = true;
        }
    }

    return result;
}

static bool dtb_bus_probe(struct bus *const bus) {
    simple_alloc_init(&g_alloc);
    bus_foreach_driver(bus, struct dtb_driver, driver.list, driver) {
        assert_msg(driver->match_flags != 0,
                   "driver " SV_FMT "'s dtb-driver is missing its match_flags "
                   "field\n",
                   SV_FMT_ARGS(driver->driver.name));

        if (driver->match_flags & __DTB_DRIVER_MATCH_COMPAT) {
            assert_msg(driver->compat_list != nullptr &&
                        driver->compat_count != 0,
                       "driver " SV_FMT "'s dtb-driver is missing its compat_* "
                       "fields\n",
                       SV_FMT_ARGS(driver->driver.name));
        }

        if (driver->match_flags & __DTB_DRIVER_MATCH_DEVICE_TYPE) {
            assert_msg(driver->device_type.length != 0,
                       "driver " SV_FMT "'s dtb-driver is missing its "
                       "device_type field\n",
                       SV_FMT_ARGS(driver->driver.name));
        }

        struct devicetree *const tree = dtb_get_tree();
        dtb_init_nodes_for_driver(driver, tree, tree->root);
    }

    return true;
}

static struct dtb_bus g_dtb_bus = {
    .bus = BUS_INIT(g_dtb_bus.bus, /*parent=*/nullptr, dtb_bus_probe),
};

__debug_optimize(3) struct dtb_bus *dtb_bus() {
    return &g_dtb_bus;
}

static void dtb_bus_init() {
    bus_register(&dtb_bus()->bus);
}

MAKE_DEV_INIT_FUNC(dtb_bus_init);
