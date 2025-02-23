/*
 * kernel/src/arch/aarch64/dev/init.c
 * © suhas pai
 */

#include "dev/dtb/bus.h"
#include "dev/dtb/init.h"
#include "dev/dtb/tree.h"

#include "sys/gic/api.h"

#include "asm/irqs.h"
#include "sys/boot.h"

void arch_init_dev() {
    if (boot_get_dtb() == nullptr) {
        return;
    }

    with_intr_disabled({
        gic_init_from_dtb();
    });

    struct devicetree *const tree = dtb_get_tree();
    bus_foreach_driver(&dtb_bus()->bus,
                       const struct dtb_driver,
                       driver.list,
                       drv)
    {
        if (sv_equals(drv->driver.name, SV_STATIC("arm-psci"))) {
            dtb_init_nodes_for_driver(drv, tree, tree->root);
            continue;
        }
    }
}

void arch_init_dev_drivers() {}
