/*
 * kernel/src/dev/time/sources/goldfish-rtc.c
 * © suhas pai
 */

#include "dev/driver.h"
#include "lib/align.h"

#include "mm/kmalloc.h"
#include "sys/mmio.h"

#include "time/clock.h"
#include "time/kstrftime.h"

struct goldfish_rtc {
    volatile uint32_t time_low;
    volatile uint32_t time_high;

    volatile uint32_t alarm_low;
    volatile uint32_t alarm_high;

    volatile uint32_t irq_enabled;

    volatile uint32_t alarm_clear;
    volatile uint32_t alarm_status;

    volatile uint32_t interrupt_clear;
} __packed;

struct goldfish_rtc_info {
    struct clock clock;
    struct mmio_region *mmio;
};

static struct goldfish_rtc_info *g_goldfish_clock = NULL;

__optimize(3) struct clock *rtc_clock_get() {
    return &g_goldfish_clock->clock;
}

__optimize(3)
static uint64_t goldfish_rtc_read(const struct clock *const clock) {
    const struct goldfish_rtc_info *const info =
        container_of(clock, struct goldfish_rtc_info, clock);

    volatile const struct goldfish_rtc *const rtc = info->mmio->base;

    const uint64_t lower = mmio_read(&rtc->time_low);
    const uint64_t higher = mmio_read(&rtc->time_high);

    return higher << 32 | lower;
}

static bool
init_from_dtb(const struct devicetree *const tree,
              const struct devicetree_node *const node)
{
    if (g_goldfish_clock != NULL) {
        printk(LOGLEVEL_WARN,
               "goldfish-rtc: multiple devices found. Ignoring\n");

        return true;
    }

    (void)tree;
    const struct devicetree_prop_reg *const reg_prop =
        (const struct devicetree_prop_reg *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_REG);

    if (reg_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "goldfish-rtc: 'reg' property in dtb node is missing\n");
        return false;
    }

    if (array_empty(reg_prop->list)) {
        printk(LOGLEVEL_WARN, "goldfish-rtc: reg property is empty\n");
        return false;
    }

    const struct devicetree_prop_reg_info *const reg =
        (const struct devicetree_prop_reg_info *)array_front(reg_prop->list);

    if (!has_align(reg->address, PAGE_SIZE)) {
        printk(LOGLEVEL_WARN,
               "goldfish-rtc: address is not aligned to page-size\n");
        return false;
    }

    uint64_t size = 0;
    if (!align_up(reg->size, PAGE_SIZE, &size)) {
        printk(LOGLEVEL_WARN,
               "goldfish-rtc: size (%" PRIu64 ") could not be aligned up to "
               "page-size\n",
               reg->size);
        return false;
    }

    const struct range phys_range = RANGE_INIT(reg->address, size);
    struct mmio_region *const mmio =
        vmap_mmio(phys_range, PROT_READ | PROT_WRITE, /*flags=*/0);

    if (mmio == NULL) {
        printk(LOGLEVEL_WARN, "goldfish-rtc: failed to mmio-map region\n");
        return false;
    }

    struct goldfish_rtc_info *const goldfish = kmalloc(sizeof(*goldfish));
    if (goldfish == NULL) {
        printk(LOGLEVEL_WARN, "goldfish-rtc: failed to alloc clock-info\n");
        vunmap_mmio(mmio);

        return false;
    }

    list_init(&goldfish->clock.list);

    goldfish->clock.name = SV_STATIC("goldfish-rtc");
    goldfish->mmio = mmio;

    goldfish->clock.read = goldfish_rtc_read;
    goldfish->clock.resolution = CLOCK_RES_NANO;

    clock_add(&goldfish->clock);
    g_goldfish_clock = goldfish;

    const uint64_t timestamp = goldfish_rtc_read(&goldfish->clock);
    printk(LOGLEVEL_INFO,
           "goldfish-rtc: init complete. ns since epoch: %" PRIu64 "\n",
           timestamp);

    const struct tm tm = tm_from_stamp(nano_to_seconds(timestamp));
    struct string string = kstrftime("%c", &tm);

    printk(LOGLEVEL_INFO,
           "goldfish-rtc: device initialized. current date&time "
           "is " STRING_FMT "\n",
           STRING_FMT_ARGS(string));

    string_destroy(&string);
    return true;
}

static const struct string_view compat[] = { SV_STATIC("google,goldfish-rtc") };
static struct dtb_driver dtb_driver = {
    .init = init_from_dtb,
    .match_flags = __DTB_DRIVER_MATCH_COMPAT,

    .compat_list = compat,
    .compat_count = countof(compat),
};

__driver static const struct driver driver = {
    .name = SV_STATIC("google,goldfish-rtc.driver"),
    .dtb = &dtb_driver,
    .pci = NULL
};