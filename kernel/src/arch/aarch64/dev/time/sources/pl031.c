/*
 * kernel/src/arch/aarch64/dev/time/soureces/pl031.c
 * © suhas pai
 */

#include "dev/driver.h"
#include "dev/printk.h"

#include "mm/mmio.h"
#include "sys/mmio.h"
#include "time/kstrftime.h"

#include "pl031.h"

struct pl031_header {
    volatile const uint32_t data;

    volatile uint32_t match;
    volatile uint32_t load;
    volatile uint32_t control;
    volatile uint32_t int_mask;

    volatile const uint32_t raw_int_status;
    volatile const uint32_t masked_int_status;

    volatile uint32_t int_clear_status; // Write-only
};

static struct mmio_region *g_mmio = NULL;
static volatile struct pl031_header *g_header = NULL;

bool
init_from_dtb(const struct devicetree *const tree,
              const struct devicetree_node *const node)
{
    if (g_mmio != NULL) {
        printk(LOGLEVEL_WARN, "pl031: device already found. ignoring\n");
        return true;
    }

    (void)tree;
    struct devicetree_prop_reg *const reg_prop =
        (struct devicetree_prop_reg *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_REG);

    if (reg_prop == NULL) {
        printk(LOGLEVEL_INFO, "pl031: dtb-node is missing 'reg' prop\n");
        return false;
    }

    const struct array reg_list = reg_prop->list;
    if (array_empty(reg_list)) {
        printk(LOGLEVEL_INFO, "pl031: dtb-node has an empty 'reg' prop\n");
        return false;
    }

    struct devicetree_prop_reg_info *const reg_info = array_front(reg_list);
    struct range reg_range = RANGE_INIT(reg_info->address, reg_info->size);

    if (!range_align_out(reg_range, PAGE_SIZE, &reg_range)) {
        printk(LOGLEVEL_INFO,
               "pl031: failed to align to page-size range of 'reg' prop of dtb "
               "node\n");
        return false;
    }

    g_mmio = vmap_mmio(reg_range, PROT_READ | PROT_WRITE, /*flags=*/0);
    if (g_mmio == NULL) {
        printk(LOGLEVEL_INFO,
               "pl031: failed to mmio-map range of 'reg' prop of dtb-node\n");
        return false;
    }

    const uint64_t offset = reg_info->address - reg_range.front;
    g_header = (volatile struct pl031_header *)(g_mmio->base + offset);

    const struct tm tm = tm_from_stamp(pl031_get_wallclock());
    struct string string = kstrftime("%c", &tm);

    printk(LOGLEVEL_INFO,
           "pl031: device initialized, current date&time is " STRING_FMT "\n",
           STRING_FMT_ARGS(string));

    string_destroy(&string);
    return true;
}

__optimize(3) sec_t pl031_get_wallclock() {
    assert_msg(g_mmio != NULL, "pl031_get_wallclock(): pl031 device not found");
    return mmio_read(&g_header->data);
}

static const struct string_view compat[] = { SV_STATIC("arm,pl031") };
static struct dtb_driver dtb_driver = {
    .init = init_from_dtb,
    .match_flags = __DTB_DRIVER_MATCH_COMPAT,

    .compat_list = compat,
    .compat_count = countof(compat),
};

__driver static const struct driver driver = {
    .name = SV_STATIC("arm,pl031.driver"),
    .dtb = &dtb_driver,
    .pci = NULL
};