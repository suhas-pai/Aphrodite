/*
 * kernel/src/arch/riscv64/dev/init.c
 * © suhas pai
 */

#include "dev/dtb/init.h"
#include "cpu/info.h"

#include "acpi/api.h"
#include "acpi/rhct.h"

#include "dev/printk.h"

#include "lib/freq.h"
#include "lib/time.h"

#include "syscon.h"

extern struct cpus_info g_cpus_info;
void arch_init_from_dtb() {
    struct devicetree *const tree = dtb_get_tree();
    struct devicetree_node *const cpus_node =
        devicetree_get_node_at_path(tree, SV_STATIC("/cpus"));

    assert_msg(cpus_node != NULL, "arch: dtb-tree is missing node \"/cpus\"\n");
    const struct devicetree_prop_other *const timebase_freq_prop =
        devicetree_node_get_other_prop(cpus_node,
                                       SV_STATIC("timebase-frequency"));

    assert_msg(timebase_freq_prop != NULL,
               "arch: dtb-node at path \"/cpus\" is missing the "
               "timebase-frequency prop\n");

    uint32_t freq = 0;
    assert_msg(devicetree_prop_other_get_u32(timebase_freq_prop, &freq),
               "arch: node at path \"/cpus\" timebase-frequency prop is of "
               "the wrong length %" PRIu32 "\n",
               timebase_freq_prop->data_length);

    assert_msg(freq >= MICRO_IN_SECONDS,
               "arch: timebase-frequency " FREQ_TO_UNIT_FMT " is too low\n",
               FREQ_TO_UNIT_FMT_ARGS_ABBREV(freq));

    g_cpus_info.timebase_frequency = freq;
}

void arch_init_dev() {
    struct devicetree *const tree = dtb_get_tree();
    if (tree != NULL) {
        static const struct string_view syscon_compat_list[] = {
            SV_STATIC("syscon")
        };

        static const struct dtb_driver syscon_dtb_driver = {
            .init = syscon_init_from_dtb,
            .match_flags = __DTB_DRIVER_MATCH_COMPAT,

            .compat_list = syscon_compat_list,
            .compat_count = countof(syscon_compat_list),
        };

        dtb_init_nodes_for_driver(&syscon_dtb_driver, tree, tree->root);

        static const struct string_view poweroff_compat_list[] = {
            SV_STATIC("syscon-poweroff")
        };

        static const struct dtb_driver poweroff_dtb_driver = {
            .init = syscon_init_reboot_dtb,
            .match_flags = __DTB_DRIVER_MATCH_COMPAT,

            .compat_list = poweroff_compat_list,
            .compat_count = countof(poweroff_compat_list),
        };

        dtb_init_nodes_for_driver(&poweroff_dtb_driver, tree, tree->root);

        static const struct string_view reboot_compat_list[] = {
            SV_STATIC("syscon-reboot")
        };

        static const struct dtb_driver reboot_dtb_driver = {
            .init = syscon_init_reboot_dtb,
            .match_flags = __DTB_DRIVER_MATCH_COMPAT,

            .compat_list = reboot_compat_list,
            .compat_count = countof(reboot_compat_list),
        };

        dtb_init_nodes_for_driver(&reboot_dtb_driver, tree, tree->root);
    }

    const struct acpi_rhct *const rhct =
        (const struct acpi_rhct *)acpi_lookup_sdt("RHCT");

    if (rhct != NULL) {
        acpi_rhct_init(rhct);
    } else {
        arch_init_from_dtb();
    }

    printk(LOGLEVEL_INFO,
           "time: frequency is " FREQ_TO_UNIT_FMT "\n",
           FREQ_TO_UNIT_FMT_ARGS_ABBREV(get_cpus_info()->timebase_frequency));
}

void arch_init_dev_drivers() {

}