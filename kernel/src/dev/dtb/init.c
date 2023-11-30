/*
 * kernel/src/dev/dtb/init.c
 * © suhas pai
 */

#include "dev/dtb/parse.h"

#include "dev/driver.h"
#include "dev/printk.h"

#include "sys/boot.h"

static struct devicetree_node g_device_tree_root = {
    .name = SV_EMPTY(),
    .parent = NULL,

    .nodeoff = 0,

    .child_list = LIST_INIT(g_device_tree_root.child_list),
    .sibling_list = LIST_INIT(g_device_tree_root.sibling_list),

    .known_props = ARRAY_INIT(sizeof(struct devicetree_prop *)),
    .other_props = ARRAY_INIT(sizeof(struct devicetree_prop_other *)),
};

static struct devicetree g_device_tree = {
    .root = &g_device_tree_root,
    .phandle_list = ARRAY_INIT(sizeof(struct devicetree_prop *))
};

static void
find_nodes_for_driver(const struct dtb_driver *const driver,
                      const struct devicetree *const tree,
                      struct devicetree_node *const node)
{
    if (driver->match_flags & __DTB_DRIVER_MATCH_COMPAT) {
        struct devicetree_prop_compat *const compat_prop =
            (struct devicetree_prop_compat *)(uint64_t)
                devicetree_node_get_prop(node, DEVICETREE_PROP_COMPAT);

        if (compat_prop == NULL) {
            goto next;
        }

        bool found = false;
        for (uint32_t i = 0; i != driver->compat_count; i++) {
            if (devicetree_prop_compat_has_sv(compat_prop,
                                              driver->compat_list[i]))
            {
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

    driver->init(tree, node);

next:
    struct devicetree_node *iter = NULL;
    list_foreach(iter, &node->child_list, sibling_list) {
        find_nodes_for_driver(driver, tree, iter);
    }
}

static void dtb_initialize_drivers() {
    driver_foreach(driver) {
        assert_msg(driver->name != NULL, "driver is missing a name");

        const struct dtb_driver *const dtb = driver->dtb;
        if (dtb == NULL) {
            continue;
        }

        assert_msg(dtb->match_flags != 0,
                   "driver %s's dtb-driver is missing its match_flags field\n",
                   driver->name);

        if (dtb->match_flags & __DTB_DRIVER_MATCH_COMPAT) {
            assert_msg(dtb->compat_list != NULL && dtb->compat_count != 0,
                       "driver %s's dtb-driver is missing its compat_* "
                       "fields\n",
                       driver->name);
        }

        if (dtb->match_flags & __DTB_DRIVER_MATCH_DEVICE_TYPE) {
            assert_msg(dtb->device_type.length != 0,
                       "driver %s's dtb-driver is missing its device_type "
                       "field\n",
                       driver->name);
        }

        find_nodes_for_driver(dtb, &g_device_tree, &g_device_tree_root);
    }
}

void dtb_init() {
    const void *const dtb = boot_get_dtb();
    if (dtb == NULL) {
        printk(LOGLEVEL_WARN, "dev: dtb not found\n");
        return;
    }

    if (!devicetree_parse(&g_device_tree, dtb)) {
        printk(LOGLEVEL_WARN, "dev: failed to parse dtb\n");
        return;
    }

    printk(LOGLEVEL_INFO, "dtb: finished initializing\n");
    dtb_initialize_drivers();
}
