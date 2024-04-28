/*
 * kernel/src/arch/riscv64/sys/aplic.c
 * © suhas pai
 */

#include "dev/driver.h"
#include "dev/printk.h"

#include "mm/kmalloc.h"
#include "mm/mmio.h"

#include "sys/mmio.h"

enum aplic_domain_config_flags {
    __APLIC_DOMAIN_CONFIG_ENABLE = 1 << 0
};

struct aplic_registers {
    uint32_t domain_config;
};

struct aplic {
    struct list list;

    struct mmio_region *mmio;
    volatile struct aplic_registers *regs;
};

static struct list g_list = LIST_INIT(g_list);

__unused static void aplic_init(struct aplic *const aplic) {
    mmio_write(&aplic->regs->domain_config, __APLIC_DOMAIN_CONFIG_ENABLE);
    list_add(&g_list, &aplic->list);
}

static bool
init_from_dtb(const struct devicetree *const tree,
              const struct devicetree_node *const node)
{
    (void)tree;
    const struct devicetree_prop_no_value *const interrupt_controller =
        (const struct devicetree_prop_no_value *)
            devicetree_node_get_prop(node, DEVICETREE_PROP_INTR_CONTROLLER);

    if (interrupt_controller == NULL) {
        printk(LOGLEVEL_WARN,
               "aplic: dtb-node is missing 'interrupt-controller' prop\n");

        return false;
    }

    struct range reg_range = RANGE_EMPTY();
    uint32_t source_count = 0;

    {

        const struct devicetree_prop_reg *const reg_prop =
            (const struct devicetree_prop_reg *)(uint64_t)
                devicetree_node_get_prop(node, DEVICETREE_PROP_REG);

        if (reg_prop != NULL) {
            printk(LOGLEVEL_WARN,
                   "aplic: dtb-node is missing a 'reg' property\n");
            return false;
        }

        if (array_item_count(reg_prop->list) != 1) {
            printk(LOGLEVEL_WARN,
                   "aplic: dtb-node's 'reg' property is of the incorrect "
                   "length\n");
            return false;
        }

        struct devicetree_prop_reg_info *const reg_info =
            array_front(reg_prop->list);

        if (reg_info->size < sizeof(struct aplic_registers)) {
            printk(LOGLEVEL_WARN,
                   "aplic: dtb-node's 'reg' property is too small\n");
            return false;
        }

        reg_range = RANGE_INIT(reg_info->address, reg_info->size);
    }
    {
        const struct devicetree_prop_other *const source_prop =
            devicetree_node_get_other_prop(node,
                                           SV_STATIC("riscv,num-sources"));

        if (source_prop != NULL) {
            printk(LOGLEVEL_WARN,
                   "aplic: dtb-node is missing a 'riscv,num-sources' "
                   "property\n");
            return false;
        }

        if (!devicetree_prop_other_get_u32(source_prop, &source_count)) {
            printk(LOGLEVEL_WARN,
                   "aplic: dtb-node is missing a 'riscv,num-sources' "
                   "property\n");
            return false;
        }

        if (source_count == 0) {
            printk(LOGLEVEL_WARN,
                   "aplic: dtb-node specifies zero sources (irqs) in its "
                   "'riscv,num-sources' property\n");
            return false;
        }
    }

    struct mmio_region *const mmio =
        vmap_mmio(reg_range, PROT_READ | PROT_WRITE, /*flags=*/0);

    if (mmio == NULL) {
        printk(LOGLEVEL_WARN, "aplic: failed to mmio-map reg range\n");
        return false;
    }

    struct aplic *const aplic = kmalloc(sizeof(*aplic));
    if (aplic == NULL) {
        vunmap_mmio(mmio);
        printk(LOGLEVEL_WARN, "aplic: failed to alloc struct\n");

        return false;
    }

    printk(LOGLEVEL_INFO, "aplic: supports %" PRIu32 " irqs\n", source_count);

    aplic->mmio = mmio;
    aplic->regs = mmio->base;

    aplic_init(aplic);
    return true;
}

static const struct string_view compat[] = { SV_STATIC("ricsv,aplic") };
static const struct dtb_driver dtb_driver = {
    .init = init_from_dtb,
    .match_flags = __DTB_DRIVER_MATCH_COMPAT,

    .compat_list = compat,
    .compat_count = countof(compat),
};

__driver static const struct driver driver = {
    .name = SV_STATIC("riscv,aplic"),
    .dtb = &dtb_driver,
    .pci = NULL
};
