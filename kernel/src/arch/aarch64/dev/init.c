/*
 * kernel/src/arch/aarch64/dev/init.c
 * © suhas pai
 */

#include "dev/dtb/gic_compat.h"
#include "dev/dtb/init.h"

#include "sys/gic.h"

void arch_init_dev() {
    struct devicetree *const tree = dtb_get_tree();
    struct dtb_driver gic_driver = {
        .init = gic_init_from_dtb,
        .match_flags = __DTB_DRIVER_MATCH_COMPAT,

        .compat_list = gic_compat_sv_list,
        .compat_count = countof(gic_compat_sv_list),
    };

    dtb_init_nodes_for_driver(&gic_driver, tree, tree->root);
}