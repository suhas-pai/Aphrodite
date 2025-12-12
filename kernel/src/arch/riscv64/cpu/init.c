/*
 * kernel/src/arch/riscv64/cpu/init.c
 * © suhas pai
 */

#include "dev/dtb/tree.h"
#include "asm/csr.h"

#include "cpu/info.h"
#include "dev/printk.h"

#include "mm/kmalloc.h"
#include "sched/thread.h"
#include "sys/boot.h"

static void setup_from_dtb(const uint32_t hartid) {
    const struct devicetree *const tree = dtb_get_tree();
    if (tree == nullptr) {
        printk(LOGLEVEL_WARN, "cpu: dtb is missing, can't get cbo/cmo info\n");
        return;
    }

    const struct devicetree_node *const cpus_node =
        devicetree_get_node_at_path(tree, SV_STATIC("/cpus"));

    assert_msg(cpus_node != nullptr, "cpu: dtb is missing a 'cpus' node");
    devicetree_node_foreach_child(cpus_node, iter) {
        const struct devicetree_prop_reg *const reg_prop =
            cast_to_ptr(const struct devicetree_prop_reg,
                        devicetree_node_get_prop(cpus_node,
                                                 DEVICETREE_PROP_REG));

        assert_msg(reg_prop != nullptr,
                   "cpu: dtb node of cpu is missing a 'reg' prop");
        assert_msg(array_item_count(reg_prop->list) == 1,
                   "cpu: 'reg' prop is of the wrong format");

        const struct devicetree_prop_reg_info *const reg_info =
            array_front(reg_prop->list, const struct devicetree_prop_reg_info);

        assert_msg(reg_info->size == 0,
                   "cpu: 'reg' prop is of the wrong format");

        if (reg_info->address != hartid) {
            continue;
        }

        const struct string_view cbo_sv = SV_STATIC("riscv,cboz-block-size");
        const struct devicetree_prop_other *const cbo_prop =
            devicetree_node_get_other_prop(cpus_node, cbo_sv);

        assert_msg(cbo_prop != nullptr,
                   "cpu: dtb node of cpu is missing a '" SV_FMT "' prop",
                   SV_FMT_ARGS(cbo_sv));

        const struct string_view cmo_sv = SV_STATIC("riscv,cbom-block-size");
        const struct devicetree_prop_other *const cmo_prop =
            devicetree_node_get_other_prop(cpus_node, cmo_sv);

        assert_msg(cmo_prop != nullptr,
                   "cpu: dtb node of cpu is missing a '" SV_FMT "' prop",
                   SV_FMT_ARGS(cmo_sv));

        uint32_t cbo_size = 0;
        uint32_t cmo_size = 0;

        assert_msg(devicetree_prop_other_get_u32(cbo_prop, &cbo_size),
                   "cpu: dtb node's cbo-size prop is malformed");
        assert_msg(devicetree_prop_other_get_u32(cmo_prop, &cmo_size),
                   "cpu: dtb node's cmo-size prop is malformed");

        assert_msg(cbo_size <= PAGE_SIZE,
                   "cpu: cbo-size is greater than page-size");

        this_cpu_mut()->cbo_size = (uint16_t)cbo_size;
        this_cpu_mut()->cmo_size = (uint16_t)cmo_size;
    }
}

extern void isr_interrupt_entry();
extern void sched_set_current_thread(struct thread *thread);

__debug_optimize(3) void cpu_early_init() {
    sched_set_current_thread(&kernel_main_thread);
    csr_write(stvec, (uint64_t)&isr_interrupt_entry);
}

__debug_optimize(3) void cpu_init_from_dtb() {
    const struct limine_mp_response *const mp_resp = boot_get_mp();
    setup_from_dtb(mp_resp->bsp_hartid);
}

__debug_optimize(3) void cpu_post_mm_init() {
    const uint64_t percpu_size = (uint64_t)(percpu_end - percpu_start);
    if (percpu_size == 0) {
        return;
    }

    this_cpu_mut()->percpu_base = kmalloc(percpu_size);
    assert_msg(this_cpu()->percpu_base != nullptr,
               "cpu: failed to alloc percpu");

    memcpy(this_cpu()->percpu_base, percpu_start, percpu_size);
}
