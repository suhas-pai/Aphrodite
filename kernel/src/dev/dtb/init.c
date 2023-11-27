/*
 * kernel/src/dev/dtb/init.c
 * © suhas pai
 */

#include "dev/dtb/parse.h"

#include "dev/driver.h"
#include "dev/printk.h"

#include "fdt/libfdt.h"
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
                      struct devicetree_node *const node)
{
    struct devicetree_prop_compat *const compat_prop =
        (struct devicetree_prop_compat *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_COMPAT);

    if (compat_prop == NULL) {
        goto next;
    }

    for (uint32_t i = 0; i != driver->compat_count; i++) {
        if (fdt_stringlist_contains(compat_prop->string.begin,
                                    compat_prop->string.length,
                                    driver->compat_list[i]))
        {
            driver->init(node);
            break;
        }
    }

next:
    struct devicetree_node *iter = NULL;
    list_foreach(iter, &node->child_list, sibling_list) {
        find_nodes_for_driver(driver, node);
    }
}

static void dtb_initialize_drivers() {
    driver_foreach(driver) {
        const struct dtb_driver *const dtb = driver->dtb;
        if (dtb == NULL) {
            continue;
        }

        find_nodes_for_driver(dtb, &g_device_tree_root);
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
