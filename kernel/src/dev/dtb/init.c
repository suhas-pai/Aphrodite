/*
 * kernel/src/dev/dtb/init.c
 * © suhas pai
 */

#include "dev/dtb/parse.h"

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

void dtb_init() {
    const void *const dtb = boot_get_dtb();
    if (dtb == NULL) {
        printk(LOGLEVEL_WARN, "dev: dtb not found\n");
        return;
    }

    if (!devicetree_parse(&g_device_tree, dtb)) {
        printk(LOGLEVEL_WARN, "dev: failed to parse dtb\n");
    }

    printk(LOGLEVEL_INFO, "dtb: finished initializing\n");
}
