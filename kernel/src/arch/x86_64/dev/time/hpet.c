/*
 * kernel/src/arch/x86_64/dev/time/hpet.c
 * © suhas pai
 */

#include "lib/adt/bitset.h"

#include "cpu/spinlock.h"
#include "dev/printk.h"

#include "lib/align.h"
#include "lib/freq.h"
#include "lib/time.h"

#include "mm/mmio.h"
#include "sched/event.h"
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

    struct hpet_addrspace_timer_info timers[32];
} __packed;

static struct mmio_region *g_hpet_mmio = NULL;
static volatile struct hpet_addrspace *g_addrspace = NULL;

static uint64_t g_frequency = 0;

static bitset_decl(g_bitset, sizeof_field(struct hpet_addrspace, timers));
static struct spinlock g_lock = SPINLOCK_INIT();

static uint8_t g_timer_count = 0;
static struct event g_bitset_event = EVENT_INIT();

__debug_optimize(3) fsec_t hpet_get_femto() {
    assert_msg(g_addrspace != NULL, "hpet: hpet_get_femto() called before init");
    return mmio_read(&g_addrspace->main_counter_value);
}

__debug_optimize(3) usec_t hpet_read() {
    return femto_to_micro(hpet_get_femto());
}

void hpet_oneshot_fsec(const fsec_t fsec) {
    uint64_t index = 0;
    SPIN_WITH_IRQ_ACQUIRED(&g_lock, {
        index = bitset_find_unset(g_bitset, /*length=*/1, /*invert=*/1);
    });

    while (index_in_bounds(index, g_timer_count)) {
        struct event *events = &g_bitset_event;
        events_await(&events,
                     /*events_count=*/1,
                     /*block=*/true,
                     /*drop_after_recv=*/true);

        index = bitset_find_unset(g_bitset, /*length=*/1, /*invert=*/1);
    }

    mmio_write(&g_addrspace->timers[index].config_and_capability, 0);
    mmio_write(&g_addrspace->timers[index].comparator_value,
               check_mul_assert(g_frequency, fsec));

    mmio_write(&g_addrspace->timers[index].config_and_capability,
               __HPET_TIMER_ENABLE_INT);
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
    if (!range_create_and_verify(hpet->base_address.address, PAGE_SIZE, &range))
    {
        printk(LOGLEVEL_WARN, "hpet: address-space's range oveflows\n");
        return;
    }

    g_hpet_mmio = vmap_mmio(range, PROT_READ | PROT_WRITE, /*flags=*/0);
    if (g_hpet_mmio == NULL) {
        printk(LOGLEVEL_WARN, "hpet: failed to mmio-map address-space\n");
        return;
    }

    g_addrspace = (volatile struct hpet_addrspace *)g_hpet_mmio->base;

    const uint64_t cap_and_id = mmio_read(&g_addrspace->general_cap_and_id);
    const uint32_t main_counter_period = cap_and_id >> 32;

    printk(LOGLEVEL_INFO,
           "hpet: period is %" PRIu64 ".%" PRIu64 " nanoseconds\n",
           femto_to_nano(main_counter_period),
           femto_mod_nano(main_counter_period));

    g_frequency = femto_period_to_hz_freq(main_counter_period);
    printk(LOGLEVEL_INFO,
           "hpet: frequency is " FREQ_TO_UNIT_FMT "\n",
           FREQ_TO_UNIT_FMT_ARGS_ABBREV(g_frequency));

    mmio_write(&g_addrspace->general_config, 0);

    g_timer_count = ((cap_and_id >> 8) & 0x1f) + 1;
    printk(LOGLEVEL_WARN, "hpet: got %" PRIu8 " timers\n", g_timer_count);

    for (volatile struct hpet_addrspace_timer_info *timer = g_addrspace->timers;
         timer != g_addrspace->timers + g_timer_count;
         timer++)
    {
        mmio_write(&timer->comparator_value, 0);
    }

    mmio_write(&g_addrspace->main_counter_value, 1);
    mmio_write(&g_addrspace->general_config, 1);
}