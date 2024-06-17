/*
 * kernel/src/dev/dtb/init.c
 * © suhas pai
 */

#include "dev/driver.h"
#include "dev/printk.h"

#include "sys/boot.h"
#include "parse.h"

static struct devicetree_node g_device_tree_root;
static struct devicetree g_device_tree;

bool
dtb_init_nodes_for_driver(const struct dtb_driver *const driver,
                          const struct devicetree *const tree,
                          const struct devicetree_node *const node)
{
    bool result = false;
    if (driver->match_flags & __DTB_DRIVER_MATCH_COMPAT) {
        struct devicetree_prop_compat *const compat_prop =
            (struct devicetree_prop_compat *)(uint64_t)
                devicetree_node_get_prop(node, DEVICETREE_PROP_COMPAT);

        if (compat_prop == NULL) {
            goto next;
        }

        bool found = false;
        for (uint32_t i = 0; i != driver->compat_count; i++) {
            const struct string_view compat_list = driver->compat_list[i];
            if (devicetree_prop_compat_has_sv(compat_prop, compat_list)) {
                found = true;
                break;
            }
        }

        if (!found) {
            goto next;
        }
    }

    if (driver->match_flags & __DTB_DRIVER_MATCH_DEVICE_TYPE) {
        struct devicetree_prop_device_type *const device_type_prop =
            (struct devicetree_prop_device_type *)(uint64_t)
                devicetree_node_get_prop(node, DEVICETREE_PROP_DEVICE_TYPE);

        if (device_type_prop == NULL) {
            goto next;
        }

        if (!sv_equals(device_type_prop->name, driver->device_type)) {
            goto next;
        }
    }

    result = driver->init(tree, node);

next:
    devicetree_node_foreach_child(node, iter) {
        if (dtb_init_nodes_for_driver(driver, tree, iter)) {
            result = true;
        }
    }

    return result;
}

static void dtb_init_drivers() {
    driver_foreach(driver) {
        assert_msg(driver->name.length != 0, "driver is missing a name");

        const struct dtb_driver *const dtb = driver->dtb;
        if (dtb == NULL) {
            continue;
        }

        assert_msg(dtb->match_flags != 0,
                   "driver " SV_FMT "'s dtb-driver is missing its match_flags "
                   "field\n",
                   SV_FMT_ARGS(driver->name));

        if (dtb->match_flags & __DTB_DRIVER_MATCH_COMPAT) {
            assert_msg(dtb->compat_list != NULL && dtb->compat_count != 0,
                       "driver " SV_FMT "'s dtb-driver is missing its compat_* "
                       "fields\n",
                       SV_FMT_ARGS(driver->name));
        }

        if (dtb->match_flags & __DTB_DRIVER_MATCH_DEVICE_TYPE) {
            assert_msg(dtb->device_type.length != 0,
                       "driver " SV_FMT "'s dtb-driver is missing its "
                       "device_type field\n",
                       SV_FMT_ARGS(driver->name));
        }

        dtb_init_nodes_for_driver(dtb, &g_device_tree, &g_device_tree_root);
    }
}

void dtb_parse_main_tree() {
    devicetree_node_init_fields(&g_device_tree_root,
                                /*parent=*/NULL,
                                /*name=*/SV_EMPTY(),
                                /*nodeoff=*/0);

    devicetree_init_fields(&g_device_tree, &g_device_tree_root);

    const void *const dtb = boot_get_dtb();
    if (dtb == NULL) {
        printk(LOGLEVEL_WARN, "dev: dtb not found\n");
        return;
    }

    assert_msg(devicetree_parse(&g_device_tree, dtb),
               "dev: failed to parse devicetree");

    printk(LOGLEVEL_INFO, "dtb: parsed main tree\n");
}

void dtb_init() {
    if (boot_get_dtb() == NULL) {
        return;
    }

    dtb_init_drivers();
    printk(LOGLEVEL_INFO, "dtb: finished initializing\n");
}

__optimize(3) struct devicetree *dtb_get_tree() {
    return &g_device_tree;
}