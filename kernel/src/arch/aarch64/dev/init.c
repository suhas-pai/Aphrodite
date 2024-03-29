/*
 * kernel/src/arch/aarch64/dev/init.c
 * © suhas pai
 */

#include "dev/dtb/gic_compat.h"
#include "dev/dtb/init.h"

#include "dev/psci.h"

#include "sys/boot.h"
#include "sys/gic.h"

void arch_init_dev() {
    struct devicetree *const tree = dtb_get_tree();
    if (boot_get_dtb() != NULL) {
        struct dtb_driver gic_driver = {
            .init = gic_init_from_dtb,
            .match_flags = __DTB_DRIVER_MATCH_COMPAT,

            .compat_list = gic_compat_sv_list,
            .compat_count = countof(gic_compat_sv_list),
        };

        assert_msg(dtb_init_nodes_for_driver(&gic_driver, tree, tree->root),
                   "dtb: gicv2 not found or was malformed");

        static const struct string_view compat_list[] = {
            SV_STATIC("arm,psci"), SV_STATIC("arm,psci-1.0"),
            SV_STATIC("arm,psci-0.2")
        };

        static const struct dtb_driver psci_dtb_driver = {
            .init = psci_init_from_dtb,
            .match_flags = __DTB_DRIVER_MATCH_COMPAT,

            .compat_list = compat_list,
            .compat_count = countof(compat_list),
        };

        assert_msg(
            dtb_init_nodes_for_driver(&psci_dtb_driver, tree, tree->root),
            "dtb: psci not found or was malformed");
    }
}