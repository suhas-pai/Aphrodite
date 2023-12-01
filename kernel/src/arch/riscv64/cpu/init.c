/*
 * kernel/src/arch/riscv64/cpu/init.c
 * © suhas pai
 */

#include "dev/dtb/init.h"
#include "dev/dtb/tree.h"

#include "cpu/info.h"
#include "dev/printk.h"
#include "sys/boot.h"

static void setup_from_dtb(const uint32_t hartid) {
    const struct devicetree *const tree = dtb_get_tree();
    if (tree == NULL) {
        printk(LOGLEVEL_WARN, "cpu: dtb is missing, can't get cbo/cmo info\n");
        return;
    }

    const struct devicetree_node *const cpus_node =
        devicetree_get_node_at_path(tree, SV_STATIC("/cpus"));

    assert_msg(cpus_node != NULL, "cpu: dtb is missing 'cpus' node");
    devicetree_node_foreach_child(cpus_node, iter) {
        const struct devicetree_prop_reg *const reg_prop =
            (const struct devicetree_prop_reg *)(uint64_t)
                devicetree_node_get_prop(cpus_node, DEVICETREE_PROP_REG);

        assert_msg(reg_prop != NULL,
                   "cpu: dtb node of cpu is missing 'reg' prop");
        assert_msg(array_item_count(reg_prop->list) == 1,
                   "cpu: 'reg' prop is of the wrong format");

        struct devicetree_prop_reg_info *const reg_info =
            (struct devicetree_prop_reg_info *)array_front(reg_prop->list);

        assert_msg(reg_info->size == 0,
                   "cpu: 'reg' prop is of the wrong format");

        if (reg_info->address != hartid) {
            continue;
        }

        const struct string_view cbo_sv = SV_STATIC("riscv,cboz-block-size");
        const struct devicetree_prop_other *const cbo_prop =
            devicetree_node_get_other_prop(cpus_node, cbo_sv);

        assert_msg(cbo_prop != NULL,
                   "cpu: dtb node of cpu is missing '" SV_FMT "' prop",
                   SV_FMT_ARGS(cbo_sv));

        const struct string_view cmo_sv = SV_STATIC("riscv,cbom-block-size");
        const struct devicetree_prop_other *const cmo_prop =
            devicetree_node_get_other_prop(cpus_node, cmo_sv);

        assert_msg(cmo_prop != NULL,
                   "cpu: dtb node of cpu is missing '" SV_FMT "' prop",
                   SV_FMT_ARGS(cmo_sv));

        get_cpu_info_mut()->cbo_size = devicetree_prop_other_get_u32(cbo_prop);
        get_cpu_info_mut()->cmo_size = devicetree_prop_other_get_u32(cmo_prop);
    }
}

void cpu_init() {
}

void cpu_init_from_dtb() {
    const struct limine_smp_response *const smp_resp = boot_get_smp();
    setup_from_dtb(smp_resp->bsp_hartid);
}