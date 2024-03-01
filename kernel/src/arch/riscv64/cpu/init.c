/*
 * kernel/src/arch/riscv64/cpu/init.c
 * © suhas pai
 */

#include "dev/dtb/tree.h"
#include "asm/csr.h"

#include "cpu/info.h"
#include "dev/printk.h"

#include "sched/thread.h"
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
            array_front(reg_prop->list);

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

        uint32_t cbo_size = 0;
        uint32_t cmo_size = 0;

        assert_msg(devicetree_prop_other_get_u32(cbo_prop, &cbo_size),
                   "cpu: dtb node's cbo-size prop is malformed");
        assert_msg(devicetree_prop_other_get_u32(cmo_prop, &cmo_size),
                   "cpu: dtb node's cmo-size prop is malformed");

        assert_msg(cbo_size < PAGE_SIZE,
                   "cpu: cbo-size is greater than page-size");

        this_cpu_mut()->cbo_size = (uint16_t)cbo_size;
        this_cpu_mut()->cmo_size = (uint16_t)cmo_size;
    }
}

extern void isr_interrupt_entry();

__optimize(3) void cpu_init() {
    csr_write(sscratch, (uint64_t)&kernel_main_thread);
    csr_write(stvec, (uint64_t)&isr_interrupt_entry);

    this_cpu_mut()->hart_id = boot_get_smp()->bsp_hartid;
}

__optimize(3) void cpu_init_from_dtb() {
    const struct limine_smp_response *const smp_resp = boot_get_smp();
    setup_from_dtb(smp_resp->bsp_hartid);
}