/*
 * kernel/src/arch/x86_64/dev/time/hpet.c
 * © suhas pai
 */

#include "dev/printk.h"

#include "lib/align.h"
#include "lib/time.h"

#include "mm/mmio.h"
#include "sys/mmio.h"

#include "hpet.h"

struct hpet_addrspace_timer_info {
    volatile uint64_t config_and_capability;
    volatile uint64_t comparator_value;
    volatile uint64_t fsb_intr_route;
} __packed;

enum hpet_addrspace_timer_flags {
    __HPET_TIMER_LEVEL_TRIGGER_INT = 1 << 1,
    __HPET_TIMER_ENABLE_INT = 1 << 2,
    __HPET_TIMER_SET_MODE_PERIODIC = 1 << 3,
    __HPET_TIMER_SUPPORTS_PERIODIC = 1 << 4,
    __HPET_TIMER_HAS_64BIT_COUNTER = 1 << 5,
    __HPET_TIMER_SET_PERIODIC_COUNTER = 1 << 6,
    __HPET_TIMER_FORCED_32BIT_COUNTER = 1 << 8,

    __HPET_TIMER_USE_FSB_INTR_MAPPING = 1 << 14,
    __HPET_TIMER_SUPPORTS_FSB_INTR_MAPPING = 1 << 15,
};

struct hpet_addrspace {
    volatile const uint64_t general_cap_and_id;
    volatile uint64_t padding_1;

    volatile uint64_t general_config;
    volatile uint64_t padding_2;

    volatile uint64_t general_intr_status;
    volatile char padding_3[200];

    volatile uint64_t main_counter_value;
    volatile uint64_t padding_4[2];

    struct hpet_addrspace_timer_info timers[31];
} __packed;

static struct mmio_region *hpet_mmio = NULL;
static volatile struct hpet_addrspace *addrspace = NULL;

__debug_optimize(3) fsec_t hpet_get_femto() {
    assert_msg(addrspace != NULL, "hpet: hpet_get_femto() called before init");
    return mmio_read(&addrspace->main_counter_value);
}

__debug_optimize(3) usec_t hpet_read() {
    return femto_to_micro(hpet_get_femto());
}

void hpet_init(const struct acpi_hpet *const hpet) {
    if (hpet->base_address.addr_space != ACPI_GAS_ADDRSPACE_KIND_SYSMEM) {
        printk(LOGLEVEL_WARN,
               "hpet: address space is not system-memory. init failed\n");
        return;
    }

    const bool has_64bit_counter =
        hpet->event_timer_block_id & __HPET_EVENTTIMER_BLOCKID_64BIT_COUNTER;

    if (!has_64bit_counter) {
        printk(LOGLEVEL_WARN,
               "hpet: does not support 64-bit counters. aborting init\n");
        return;
    }

    if (!has_align(hpet->base_address.address, PAGE_SIZE)) {
        printk(LOGLEVEL_WARN,
               "hpet: address-space is not aligned to page size\n");
        return;
    }

    struct range range = RANGE_EMPTY();
    if (!range_create_and_verify(hpet->base_address.address,
                                 PAGE_SIZE,
                                 &range))
    {
        printk(LOGLEVEL_WARN, "hpet: address-space's range oveflows\n");
        return;
    }

    hpet_mmio = vmap_mmio(range, PROT_READ | PROT_WRITE, /*flags=*/0);
    if (hpet_mmio == NULL) {
        printk(LOGLEVEL_WARN, "hpet: failed to mmio-map address-space\n");
        return;
    }

    addrspace = (volatile struct hpet_addrspace *)hpet_mmio->base;

    const uint64_t cap_and_id = mmio_read(&addrspace->general_cap_and_id);
    const uint32_t main_counter_period = cap_and_id >> 32;

    printk(LOGLEVEL_INFO,
           "hpet: period is %" PRIu32 ".%" PRIu32 " nanoseconds\n",
           femto_to_nano(main_counter_period),
           femto_mod_nano(main_counter_period));

    mmio_write(&addrspace->general_config, 0);

    const uint8_t timer_count = (cap_and_id >> 8) & 0x1f;
    printk(LOGLEVEL_WARN, "hpet: got %" PRIu8 " timers\n", timer_count);

    for (volatile struct hpet_addrspace_timer_info *timer = addrspace->timers;
         timer != addrspace->timers + timer_count;
         timer++)
    {
        mmio_write(&timer->comparator_value, 0);
    }

    mmio_write(&addrspace->main_counter_value, 1);
    mmio_write(&addrspace->general_config, 1);
}