/*
 * kernel/src/dev/dtb/init.c
 * © suhas pai
 */

#include "dev/dtb/parse.h"

#include "dev/printk.h"
#include "sys/boot.h"

static struct devicetree_node g_device_tree_root;
static struct devicetree g_device_tree;

void dtb_parse_main_tree() {
    devicetree_node_init_fields(&g_device_tree_root,
                                /*parent=*/nullptr,
                                /*name=*/SV_EMPTY(),
                                /*nodeoff=*/0);

    devicetree_init_fields(&g_device_tree, &g_device_tree_root);

    const void *const dtb = boot_get_dtb();
    if (dtb == nullptr) {
        printk(LOGLEVEL_WARN, "dev: dtb not found\n");
        return;
    }

    assert_msg(devicetree_parse(&g_device_tree, dtb),
               "dev: failed to parse devicetree");

    printk(LOGLEVEL_INFO, "dtb: parsed main tree\n");
}

__debug_optimize(3) struct devicetree *dtb_get_tree() {
    return &g_device_tree;
}