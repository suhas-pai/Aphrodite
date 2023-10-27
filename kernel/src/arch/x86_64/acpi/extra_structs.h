/*
 * kernel/arch/x86_64/acpi/extra_structs.h
 * © suhas pai
 */

#pragma once
#include "acpi/structs.h"

enum acpi_hpet_event_timer_block_id_flags {
    __HPET_EVENTTIMER_BLOCKID_HW_REV_ID = (1ull << 8) - 1,
    __HPET_EVENTTIMER_BLOCKID_COMPARATOR_NUM_1ST_TIMER = 0b1111 << 8,
    __HPET_EVENTTIMER_BLOCKID_64BIT_COUNTER = 1ull << 13,
    __HPET_EVENTTIMER_BLOCKID_LEG_REPLACE_IRQ_ROUTECAPABLE = 1ull << 15,
    __HPET_EVENTTIMER_BLOCKID_1ST_TIMER_PCI_VENDOR_ID =
        ((1ull << 32) - 1) << 32
};

enum acpi_hpet_page_prot_guarantee {
    ACPI_HPET_PAGE_PROT_NONE,
    ACPI_HPET_PAGE_PROT_4K,
    ACPI_HPET_PAGE_PROT_64K,
};

enum acpi_hpet_page_prot_and_oem_attr_flags {
    __HPET_PAGEPROT_OEMATTR_PAGE_HW_CAP = 0b1111,
    __HPET_PAGEPROT_OEMATTR_OEM_ATTR = 0b1111 << 4
};

struct acpi_hpet {
    struct acpi_sdt sdt;
    uint32_t event_timer_block_id;
    struct acpi_gas base_address;
    uint8_t hpet_number;
    uint16_t main_counter_min_clock_tick_periodic;
    uint8_t page_prot_and_oem_attr;
} __packed;