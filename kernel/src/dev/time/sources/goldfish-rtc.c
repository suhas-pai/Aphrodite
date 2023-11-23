/*
 * kernel/src/dev/time/sources/goldfish-rtc.c
 * © suhas pai
 */

#include "dev/dtb/dtb.h"

#include "dev/driver.h"
#include "dev/printk.h"

#include "lib/align.h"

#include "mm/kmalloc.h"
#include "mm/mmio.h"

#include "sys/mmio.h"

#include "time/kstrftime.h"
#include "time/time.h"

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
    struct clock_source clock_source;
    struct mmio_region *mmio;
};

static uint64_t goldfish_rtc_read(struct clock_source *const clock_source) {
    const struct goldfish_rtc_info *const info =
        container_of(clock_source, struct goldfish_rtc_info, clock_source);

    volatile const struct goldfish_rtc *const rtc = info->mmio->base;

    const uint64_t lower = mmio_read(&rtc->time_low);
    const uint64_t higher = mmio_read(&rtc->time_high);

    return nano_to_micro(higher << 32 | lower);
}

static
bool goldfish_rtc_init_from_dtb(const void *const dtb, const int nodeoff) {
    struct dtb_addr_size_pair base_addr_reg = DTB_ADDR_SIZE_PAIR_INIT();
    uint32_t pair_count = 1;

    const bool get_base_addr_reg_result =
        dtb_get_reg_pairs(dtb,
                          nodeoff,
                          /*start_index=*/0,
                          &pair_count,
                          &base_addr_reg);

    if (!get_base_addr_reg_result) {
    #if defined(__x86_64__)
        verify_not_reached();
    #else
        printk(LOGLEVEL_WARN,
               "goldfish-rtc: base-addr reg of 'reg' property of "
               "google,goldfish-rtc dtb-node is malformed\n");
        return false;
    #endif
    }

    if (!has_align(base_addr_reg.address, PAGE_SIZE)) {
        printk(LOGLEVEL_WARN,
               "goldfish-rtc: address is not aligned to page-size\n");
        return false;
    }

    uint64_t size = 0;
    if (!align_up(base_addr_reg.size, PAGE_SIZE, &size)) {
        printk(LOGLEVEL_WARN,
               "goldfish-rtc: size (%" PRIu64 ") could not be aligned up to "
               "page-size\n",
               base_addr_reg.size);
        return false;
    }

    const struct range phys_range = RANGE_INIT(base_addr_reg.address, size);
    struct mmio_region *const mmio =
        vmap_mmio(phys_range, PROT_READ | PROT_WRITE, /*flags=*/0);

    if (mmio == NULL) {
        printk(LOGLEVEL_WARN, "goldfish-rtc: failed to mmio-map region\n");
        return false;
    }

    struct goldfish_rtc_info *const clock = kmalloc(sizeof(*clock));
    if (clock == NULL) {
        printk(LOGLEVEL_WARN, "goldfish-rtc: failed to alloc clock-info\n");
        vunmap_mmio(mmio);

        return false;
    }

    list_init(&clock->clock_source.list);

    clock->clock_source.name = "goldfish-rtc";
    clock->mmio = mmio;
    clock->clock_source.read = goldfish_rtc_read;

    add_clock_source(&clock->clock_source);

    const uint64_t timestamp = clock->clock_source.read(&clock->clock_source);
    printk(LOGLEVEL_INFO,
           "goldfish-rtc: init complete. ns since epoch: %" PRIu64 "\n",
           timestamp);

    const struct tm tm = tm_from_stamp(nano_to_seconds(timestamp));
    struct string string = kstrftime("%c", &tm);

    printk(LOGLEVEL_INFO,
           "goldfish-rtc: current date&time is " STRING_FMT "\n",
           STRING_FMT_ARGS(string));

    string_destroy(&string);
    return true;
}

static const char *const compat[] = { "google,goldfish-rtc" };
static struct dtb_driver dtb_driver = {
    .compat_list = compat,
    .compat_count = countof(compat),
    .init = goldfish_rtc_init_from_dtb
};

__driver static const struct driver driver = {
    .dtb = &dtb_driver,
    .pci = NULL
};