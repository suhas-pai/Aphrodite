/*
 * kernel/src/arch/riscv64/dev/syscon.c
 * © suhas pai
 */

#include "cpu/util.h"
#include "dev/driver.h"
#include "dev/printk.h"

#include "lib/util.h"
#include "mm/mmio.h"
#include "sys/mmio.h"

enum syscon_method {
    SYSCON_METH_POWEROFF,
    SYSCON_METH_REBOOT
};

struct syscon_method_info {
    uint32_t phandle;
    uint32_t offset;
    uint32_t value;
};

static struct mmio_region *g_mmio = NULL;
static struct syscon_method_info g_method_info[] = {
    [SYSCON_METH_POWEROFF] = {
        .phandle = 0,
        .offset = 0,
        .value = 0
    },
    [SYSCON_METH_REBOOT] = {
        .phandle = 0,
        .offset = 0,
        .value = 0
    }
};

#define SYSCON_MMIO_SIZE 0x1000

void syscon_poweroff() {
    const struct syscon_method_info *const meth_info =
        &g_method_info[SYSCON_METH_POWEROFF];

    if (meth_info->phandle == 0) {
        printk(LOGLEVEL_WARN, "syscon: poweroff method not implemented\n");
        cpu_halt();
    }

    mmio_write(g_mmio->base + meth_info->offset, meth_info->value);
}

void syscon_reboot() {
    const struct syscon_method_info *const meth_info =
        &g_method_info[SYSCON_METH_REBOOT];

    if (meth_info->phandle == 0) {
        printk(LOGLEVEL_WARN, "syscon: reboot method not implemented\n");
        cpu_halt();
    }

    mmio_write(g_mmio->base + meth_info->offset, meth_info->value);
}

static bool
init_syscon_dtb(const struct devicetree *const tree,
                const struct devicetree_node *const node)
{
    if (g_mmio != NULL) {
        printk(LOGLEVEL_WARN,
               "syscon: multiple syscons found, ignoring one found in dtb\n");
        return true;
    }

    (void)tree;
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

    const struct devicetree_prop_phandle *const phandle_prop =
        (const struct devicetree_prop_phandle *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_PHANDLE);

    if (phandle_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "syscon: dtb-node is missing a 'phandle' property\n");
        return false;
    }

    struct devicetree_prop_reg_info *const reg_info =
        array_front(reg_prop->list);

    if (reg_info->size != SYSCON_MMIO_SIZE) {
        printk(LOGLEVEL_WARN,
               "syscon: dtb-node's 'reg' property is of the incorrect "
               "length\n");
        return false;
    }

    g_mmio =
        vmap_mmio(RANGE_INIT(reg_info->address, reg_info->size),
                  PROT_READ | PROT_WRITE,
                  /*flags=*/0);

    if (g_mmio == NULL) {
        printk(LOGLEVEL_WARN, "syscon: failed to mmio-map reg range\n");
        return false;
    }

    return false;
}

static bool
init_poweroff_dtb(const struct devicetree *const tree,
                  const struct devicetree_node *const node)
{
    assert_msg(g_method_info[SYSCON_METH_POWEROFF].phandle == 0,
               "syscon: multiple methods for syscon-poweroff found");

    const struct devicetree_prop_other *const regmap_prop =
        devicetree_node_get_other_prop(node, SV_STATIC("regmap"));

    if (regmap_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "syscon: syscon-poweroff dtb-node is missing a 'regmap' prop\n");
        return false;
    }

    const uint32_t phandle = devicetree_prop_other_get_u32(regmap_prop);
    const struct devicetree_node *const phandle_node =
        devicetree_get_node_for_phandle(tree, phandle);

    if (phandle_node == NULL) {
        printk(LOGLEVEL_WARN,
               "syscon: syscon-poweroff dtb-node has an invalid phandle (in "
               "regmap)\n");
        return false;
    }

    if (!devicetree_node_has_compat_sv(phandle_node, SV_STATIC("syscon"))) {
        printk(LOGLEVEL_WARN,
               "syscon: syscon-poweroff dtb-node's phandle points to a node "
               "that isn't a syscon node\n");
        return false;
    }

    const struct devicetree_prop_other *const offset_prop =
        devicetree_node_get_other_prop(node, SV_STATIC("offset"));

    if (offset_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "syscon: syscon-poweroff dtb-node is missing an 'offset' prop\n");
        return false;
    }

    const struct devicetree_prop_other *const value_prop =
        devicetree_node_get_other_prop(node, SV_STATIC("value"));

    if (value_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "syscon: syscon-poweroff dtb-node is missing a 'value' prop\n");
        return false;
    }

    const uint32_t offset = devicetree_prop_other_get_u32(offset_prop);
    const struct range range = RANGE_INIT(offset, sizeof(uint32_t));

    if (!index_range_in_bounds(range, SYSCON_MMIO_SIZE)) {
        printk(LOGLEVEL_WARN,
               "syscon: syscon-poweroff dtb-node's offset falls outside "
               "syscon's mmio range\n");
        return false;
    }

    g_method_info[SYSCON_METH_POWEROFF].phandle = phandle;
    g_method_info[SYSCON_METH_POWEROFF].offset = offset;
    g_method_info[SYSCON_METH_POWEROFF].value =
        devicetree_prop_other_get_u32(value_prop);

    return true;
}

static bool
init_reboot_dtb(const struct devicetree *const tree,
                const struct devicetree_node *const node)
{
    assert_msg(g_method_info[SYSCON_METH_REBOOT].phandle == 0,
               "syscon: multiple methods for syscon-reboot found");

    const struct devicetree_prop_other *const regmap_prop =
        devicetree_node_get_other_prop(node, SV_STATIC("regmap"));

    if (regmap_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "syscon: syscon-reboot dtb-node is missing a 'regmap' prop\n");
        return false;
    }

    const uint32_t phandle = devicetree_prop_other_get_u32(regmap_prop);
    const struct devicetree_node *const phandle_node =
        devicetree_get_node_for_phandle(tree, phandle);

    if (phandle_node == NULL) {
        printk(LOGLEVEL_WARN,
               "syscon: syscon-reboot dtb-node has an invalid phandle (in "
               "regmap)\n");
        return false;
    }

    if (!devicetree_node_has_compat_sv(phandle_node, SV_STATIC("syscon"))) {
        printk(LOGLEVEL_WARN,
               "syscon: syscon-reboot dtb-node's phandle points to a node that "
               "isn't a syscon node\n");
        return false;
    }

    const struct devicetree_prop_other *const offset_prop =
        devicetree_node_get_other_prop(node, SV_STATIC("offset"));

    if (offset_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "syscon: syscon-reboot dtb-node is missing an 'offset' prop\n");
        return false;
    }

    const struct devicetree_prop_other *const value_prop =
        devicetree_node_get_other_prop(node, SV_STATIC("value"));

    if (value_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "syscon: syscon-reboot dtb-node is missing a 'value' prop\n");
        return false;
    }

    const uint32_t offset = devicetree_prop_other_get_u32(offset_prop);
    const struct range range = RANGE_INIT(offset, sizeof(uint32_t));

    if (!index_range_in_bounds(range, SYSCON_MMIO_SIZE)) {
        printk(LOGLEVEL_WARN,
               "syscon: syscon-reboot dtb-node's offset falls outside "
               "syscon's mmio range\n");
        return false;
    }

    g_method_info[SYSCON_METH_REBOOT].phandle = phandle;
    g_method_info[SYSCON_METH_REBOOT].offset = offset;
    g_method_info[SYSCON_METH_REBOOT].value =
        devicetree_prop_other_get_u32(value_prop);

    return true;
}

static const struct string_view reboot_compat_list[] = {
    SV_STATIC("syscon-reboot")
};

static const struct dtb_driver reboot_dtb_driver = {
    .init = init_reboot_dtb,
    .match_flags = __DTB_DRIVER_MATCH_COMPAT,

    .compat_list = reboot_compat_list,
    .compat_count = countof(reboot_compat_list),
};

__driver static const struct driver reboot_driver = {
    .name = "riscv64-syscon-reboot-driver",
    .dtb = &reboot_dtb_driver,
    .pci = NULL
};


static const struct string_view poweroff_compat_list[] = {
    SV_STATIC("syscon-poweroff")
};

static const struct dtb_driver poweroff_dtb_driver = {
    .init = init_poweroff_dtb,
    .match_flags = __DTB_DRIVER_MATCH_COMPAT,

    .compat_list = poweroff_compat_list,
    .compat_count = countof(poweroff_compat_list),
};

__driver static const struct driver poweroff_driver = {
    .name = "riscv64-syscon-poweroff-driver",
    .dtb = &poweroff_dtb_driver,
    .pci = NULL
};

static const struct string_view syscon_compat_list[] = {
    SV_STATIC("syscon")
};

static const struct dtb_driver syscon_dtb_driver = {
    .init = init_syscon_dtb,
    .match_flags = __DTB_DRIVER_MATCH_COMPAT,

    .compat_list = syscon_compat_list,
    .compat_count = countof(syscon_compat_list),
};

__driver static const struct driver syscon_driver = {
    .name = "riscv64-syscon-driver",
    .dtb = &syscon_dtb_driver,
    .pci = NULL
};