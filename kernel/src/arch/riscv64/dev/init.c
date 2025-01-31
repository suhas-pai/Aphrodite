/*
 * kernel/src/arch/riscv64/dev/init.c
 * © suhas pai
 */

#include <lib/freq.h>
#include <lib/time.h>

#include "dev/dtb/bus.h"
#include "dev/dtb/init.h"
#include "dev/dtb/tree.h"

#include "acpi/api.h"
#include "acpi/rhct.h"

#include "cpu/info.h"

#include "dev/printk.h"
#include "dev/syscon.h"

extern struct cpus_info g_cpus_info;
void arch_init_from_dtb() {
    struct devicetree *const tree = dtb_get_tree();
    struct devicetree_node *const cpus_node =
        devicetree_get_node_at_path(tree, SV_STATIC("/cpus"));

    assert_msg(cpus_node != nullptr, "arch: dtb-tree is missing node \"/cpus\"\n");
    const struct devicetree_prop_other *const timebase_freq_prop =
        devicetree_node_get_other_prop(cpus_node,
                                       SV_STATIC("timebase-frequency"));

    assert_msg(timebase_freq_prop != nullptr,
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
    uint8_t init_count = 0;

    if (tree != nullptr) {
        bus_foreach_driver(&dtb_bus()->bus,
                           struct dtb_driver,
                           driver.list,
                           drv)
        {
            if (sv_equals(drv->driver.name, SV_STATIC("riscv64-syscon"))) {
                dtb_init_nodes_for_driver(drv, tree, tree->root);
                init_count |= 1 << 0;

                if (init_count == 0b111) {
                    break;
                }
            }

            const struct string_view poweroff_sv =
                SV_STATIC("riscv64-syscon-poweroff");

            if (sv_equals(drv->driver.name, poweroff_sv)) {
                dtb_init_nodes_for_driver(drv, tree, tree->root);
                init_count |= 1 << 1;

                if (init_count == 0b111) {
                    break;
                }
            }

            const struct string_view reboot_sv =
                SV_STATIC("riscv64-syscon-reboot");

            if (sv_equals(drv->driver.name, reboot_sv)) {
                dtb_init_nodes_for_driver(drv, tree, tree->root);
                init_count |= 1 << 2;

                if (init_count == 0b111) {
                    break;
                }
            }
        }
    }

    const auto rhct =
        (const struct os_acpi_rhct *)acpi_lookup_sdt("RHCT");

    if (rhct != nullptr) {
        acpi_rhct_init(rhct);
    } else {
        arch_init_from_dtb();
    }

    printk(LOGLEVEL_INFO,
           "time: frequency is " FREQ_TO_UNIT_FMT "\n",
           FREQ_TO_UNIT_FMT_ARGS_ABBREV(get_cpus_info()->timebase_frequency));
}

void arch_init_dev_drivers() {}
