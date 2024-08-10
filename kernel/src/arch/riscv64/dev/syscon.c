/*
 * kernel/src/arch/riscv64/dev/syscon.c
 * © suhas pai
 */

#include "dev/dtb/tree.h"

#include "cpu/util.h"
#include "dev/printk.h"

#include "lib/util.h"

#include "mm/mmio.h"
#include "sys/mmio.h"

enum syscon_method {
    SYSCON_METHOD_POWEROFF,
    SYSCON_METHOD_REBOOT
};

__debug_optimize(3) static inline
const char *syscon_method_get_string(const enum syscon_method method) {
    switch (method) {
        case SYSCON_METHOD_POWEROFF:
            return "poweroff";
        case SYSCON_METHOD_REBOOT:
            return "reboot";
    }

    verify_not_reached();
}

struct syscon_method_info {
    uint32_t phandle;
    uint32_t offset;
    uint32_t value;
};

static struct mmio_region *g_mmio = NULL;
static struct syscon_method_info g_method_info[] = {
    [SYSCON_METHOD_POWEROFF] = {
        .phandle = 0,
        .offset = 0,
        .value = 0
    },
    [SYSCON_METHOD_REBOOT] = {
        .phandle = 0,
        .offset = 0,
        .value = 0
    }
};

#define SYSCON_MMIO_SIZE 0x1000

__debug_optimize(3) void syscon_poweroff() {
    const struct syscon_method_info *const meth_info =
        &g_method_info[SYSCON_METHOD_POWEROFF];

    if (meth_info->phandle == 0) {
        printk(LOGLEVEL_WARN, "syscon: poweroff method not implemented\n");
        cpu_idle();
    }

    mmio_write((volatile uint32_t *)(g_mmio->base + meth_info->offset),
               meth_info->value);
}

__debug_optimize(3) void syscon_reboot() {
    const struct syscon_method_info *const meth_info =
        &g_method_info[SYSCON_METHOD_REBOOT];

    if (meth_info->phandle == 0) {
        printk(LOGLEVEL_WARN, "syscon: reboot method not implemented\n");
        cpu_idle();
    }

    mmio_write((volatile uint32_t *)(g_mmio->base + meth_info->offset),
               meth_info->value);
}

bool
syscon_init_from_dtb(const struct devicetree *const tree,
                     const struct devicetree_node *const node)
{
    (void)tree;
    if (g_mmio != NULL) {
        printk(LOGLEVEL_WARN,
               "syscon: multiple syscons found, ignoring one found in dtb\n");
        return true;
    }

    struct range reg_range = RANGE_EMPTY();
    {
        const struct devicetree_prop_reg *const reg_prop =
            (const struct devicetree_prop_reg *)(uint64_t)
                devicetree_node_get_prop(node, DEVICETREE_PROP_REG);

        if (reg_prop != NULL) {
            printk(LOGLEVEL_WARN,
                   "syscon: dtb-node is missing a 'reg' property\n");
            return false;
        }

        if (array_item_count(reg_prop->list) != 1) {
            printk(LOGLEVEL_WARN,
                   "syscon: dtb-node's 'reg' property is of the incorrect "
                   "length\n");
            return false;
        }

        struct devicetree_prop_reg_info *const reg= array_front(reg_prop->list);
        if (reg->size != SYSCON_MMIO_SIZE) {
            printk(LOGLEVEL_WARN,
                   "syscon: dtb-node's 'reg' property is of the incorrect "
                   "length\n");
            return false;
        }

        if (!range_create_and_verify(reg->address, reg->size, &reg_range)) {
            printk(LOGLEVEL_WARN,
                   "syscon: dtb-node's 'reg' property overflows\n");
            return false;
        }
    }

    const struct devicetree_prop_phandle *const phandle_prop =
        (const struct devicetree_prop_phandle *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_PHANDLE);

    if (phandle_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "syscon: dtb-node is missing a 'phandle' property\n");
        return false;
    }

    g_mmio = vmap_mmio(reg_range, PROT_READ | PROT_WRITE, /*flags=*/0);
    if (g_mmio == NULL) {
        printk(LOGLEVEL_WARN, "syscon: failed to mmio-map reg range\n");
        return false;
    }

    return true;
}

static bool
init_method_from_dtb(const struct devicetree *const tree,
                     const struct devicetree_node *const node,
                     const enum syscon_method method)
{
    assert_msg(g_method_info[method].phandle == 0,
               "syscon: multiple methods for syscon-%s found",
               syscon_method_get_string(method));

    uint32_t phandle = 0;
    {
        const struct devicetree_prop_other *const regmap_prop =
            devicetree_node_get_other_prop(node, SV_STATIC("regmap"));

        if (regmap_prop == NULL) {
            printk(LOGLEVEL_WARN,
                   "syscon: syscon-%s dtb-node is missing a 'regmap' prop\n",
                   syscon_method_get_string(method));
            return false;
        }

        if (!devicetree_prop_other_get_u32(regmap_prop, &phandle)) {
            printk(LOGLEVEL_WARN,
                   "syscon: syscon-%s dtb-node has a malformed 'regmap' prop\n",
                   syscon_method_get_string(method));
            return false;
        }

        const struct devicetree_node *const phandle_node =
            devicetree_get_node_for_phandle(tree, phandle);

        if (phandle_node == NULL) {
            printk(LOGLEVEL_WARN,
                   "syscon: syscon-%s dtb-node has an invalid phandle (in "
                   "regmap)\n",
                   syscon_method_get_string(method));
            return false;
        }

        if (!devicetree_node_has_compat_sv(phandle_node, SV_STATIC("syscon"))) {
            printk(LOGLEVEL_WARN,
                   "syscon: syscon-%s dtb-node's phandle points to a node that "
                   "isn't a syscon node\n",
                   syscon_method_get_string(method));
            return false;
        }
    }

    uint32_t offset = 0;
    {
        const struct devicetree_prop_other *const offset_prop =
            devicetree_node_get_other_prop(node, SV_STATIC("offset"));

        if (offset_prop == NULL) {
            printk(LOGLEVEL_WARN,
                   "syscon: syscon-%s dtb-node is missing an 'offset' prop\n",
                   syscon_method_get_string(method));
            return false;
        }

        if (!devicetree_prop_other_get_u32(offset_prop, &offset)) {
            printk(LOGLEVEL_WARN,
                   "syscon: syscon-%s dtb-node has a malformed 'offset' prop\n",
                   syscon_method_get_string(method));
            return false;
        }

        if (!index_range_in_bounds(RANGE_INIT(offset, sizeof(uint32_t)),
                                   g_mmio->size))
        {

            printk(LOGLEVEL_WARN,
                   "syscon: syscon-%s dtb-node's 'offset' is past end of "
                   "syscon mmio-space\n",
                   syscon_method_get_string(method));
            return false;
        }
    }

    uint32_t value = 0;
    {
        const struct devicetree_prop_other *const value_prop =
            devicetree_node_get_other_prop(node, SV_STATIC("value"));

        if (value_prop == NULL) {
            printk(LOGLEVEL_WARN,
                   "syscon: syscon-%s dtb-node is missing a 'value' prop\n",
                   syscon_method_get_string(method));
            return false;
        }

        if (!devicetree_prop_other_get_u32(value_prop, &value)) {
            printk(LOGLEVEL_WARN,
                   "syscon: syscon-%s dtb-node has a malformed 'value' prop\n",
                   syscon_method_get_string(method));
            return false;
        }

        const struct range range = RANGE_INIT(offset, sizeof(uint32_t));
        if (!index_range_in_bounds(range, SYSCON_MMIO_SIZE)) {
            printk(LOGLEVEL_WARN,
                   "syscon: syscon-%s dtb-node's offset falls outside syscon's "
                   "mmio range\n",
                   syscon_method_get_string(method));
            return false;
        }
    }

    g_method_info[method].phandle = phandle;
    g_method_info[method].offset = offset;
    g_method_info[method].value = value;

    return true;
}

__debug_optimize(3) bool
syscon_init_poweroff_dtb(const struct devicetree *const tree,
                         const struct devicetree_node *const node)
{
    return init_method_from_dtb(tree, node, SYSCON_METHOD_POWEROFF);
}

__debug_optimize(3) bool
syscon_init_reboot_dtb(const struct devicetree *const tree,
                       const struct devicetree_node *const node)
{
    return init_method_from_dtb(tree, node, SYSCON_METHOD_REBOOT);
}
