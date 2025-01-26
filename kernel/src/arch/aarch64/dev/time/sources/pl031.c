/*
 * kernel/src/arch/aarch64/dev/time/sources/pl031.c
 * © suhas pai
 */

#include "dev/time/sources/pl031.h"

#include "dev/dtb/bus.h"
#include "dev/dtb/device.h"
#include "dev/dtb/driver.h"

#include "dev/init.h"
#include "mm/mmio.h"

#include "sys/mmio.h"
#include "time/kstrftime.h"

struct pl031_header {
    volatile const uint32_t data;

    volatile uint32_t match;
    volatile uint32_t load;
    volatile uint32_t control;
    volatile uint32_t intr_mask;

    volatile const uint32_t raw_intr_status;
    volatile const uint32_t masked_intr_status;

    volatile uint32_t intr_clear_status; // Write-only
};

static struct mmio_region *g_mmio = nullptr;
static volatile struct pl031_header *g_header = nullptr;

static bool pl031_dtb_probe(struct device *const the_device) {
    if (g_mmio != nullptr) {
        printk(LOGLEVEL_WARN, "pl031: device already found. ignoring\n");
        return true;
    }

    const struct dtb_device *const device =
        parent_of(the_device, struct dtb_device, device);

    const struct devicetree_node *const node = device->node;
    const struct devicetree_prop_reg *const reg_prop =
        (const struct devicetree_prop_reg *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_REG);

    if (reg_prop == nullptr) {
        printk(LOGLEVEL_INFO, "pl031: dtb-node is missing a 'reg' prop\n");
        return false;
    }

    const struct array reg_list = reg_prop->list;
    if (array_empty(reg_list)) {
        printk(LOGLEVEL_INFO, "pl031: dtb-node has an empty 'reg' prop\n");
        return false;
    }

    struct devicetree_prop_reg_info *const reg_info =
        array_front(&reg_list, struct devicetree_prop_reg_info);

    struct range reg_range = RANGE_EMPTY();
    if (!range_create_and_verify(reg_info->address,
                                 reg_info->size,
                                 &reg_range))
    {
        printk(LOGLEVEL_INFO, "pl031: dtb-node's 'reg' prop range overflows\n");
        return false;
    }

    if (!range_align_out(reg_range, PAGE_SIZE, &reg_range)) {
        printk(LOGLEVEL_INFO,
               "pl031: failed to align to page-size range of 'reg' prop of dtb "
               "node\n");
        return false;
    }

    g_mmio = vmap_mmio(reg_range, PROT_READ | PROT_WRITE, /*flags=*/0);
    if (g_mmio == nullptr) {
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

__debug_optimize(3) sec_t pl031_get_wallclock() {
    assert_msg(g_mmio != nullptr,
               "pl031_get_wallclock(): pl031 device not found");
    return mmio_read(&g_header->data);
}

static void init_drivers() {
    static const struct string_view compat[] = { SV_STATIC("arm,pl031") };
    static struct dtb_driver dtb_driver = {
        .match_flags = __DTB_DRIVER_MATCH_COMPAT,

        .compat_list = compat,
        .compat_count = countof(compat),
    };

    driver_initialize(&dtb_driver.driver,
                      &dtb_bus()->bus,
                      /*name=*/SV_STATIC("arm-pl031"),
                      pl031_dtb_probe,
                      /*remove=*/nullptr,
                      /*shutdown=*/nullptr,
                      /*suspend=*/nullptr,
                      /*resume=*/nullptr);
}

MAKE_DEV_INIT_FUNC(init_drivers);
