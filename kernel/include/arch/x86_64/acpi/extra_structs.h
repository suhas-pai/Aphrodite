/*
 * kernel/src/arch/x86_64/acpi/extra_structs.h
 * © suhas pai
 */

#pragma once
#include "acpi/structs.h"

enum os_acpi_hpet_event_timer_block_id_shifts : uint8_t {
    HPET_EVENT_TIMER_BLOCKID_COMPARATOR_NUM_1ST_TIMER_SHIFT = 8,
    HPET_EVENT_TIMER_BLOCKID_1ST_TIMER_PCI_VENDOR_ID_SHIFT = 32,
};

enum os_acpi_hpet_event_timer_block_id_flags : uint64_t {
    __HPET_EVENT_TIMER_BLOCKID_HW_REV_ID = 0xFF,
    __HPET_EVENT_TIMER_BLOCKID_COMPARATOR_NUM_1ST_TIMER =
        0b1111 << HPET_EVENT_TIMER_BLOCKID_COMPARATOR_NUM_1ST_TIMER_SHIFT,
    __HPET_EVENT_TIMER_BLOCKID_64BIT_COUNTER = 1ull << 13,
    __HPET_EVENT_TIMER_BLOCKID_LEG_REPLACE_IRQ_ROUTE_CAPABLE = 1ull << 15,
    __HPET_EVENT_TIMER_BLOCKID_1ST_TIMER_PCI_VENDOR_ID =
        0xFFFFFFFFull << HPET_EVENT_TIMER_BLOCKID_1ST_TIMER_PCI_VENDOR_ID_SHIFT
};

enum os_acpi_hpet_page_prot_guarantee : uint8_t {
    ACPI_HPET_PAGE_PROT_NONE,
    ACPI_HPET_PAGE_PROT_4K,
    ACPI_HPET_PAGE_PROT_64K,
};

enum os_acpi_hpet_page_prot_and_oem_attr_flags : uint8_t {
    __HPET_PAGEPROT_OEMATTR_PAGE_HW_CAP = 0b1111,
    __HPET_PAGEPROT_OEMATTR_OEM_ATTR = 0b1111 << 4
};

struct os_acpi_hpet {
    struct os_acpi_sdt sdt;
    uint32_t event_timer_block_id;
    struct os_acpi_gas base_address;
    uint8_t hpet_number;
    uint16_t main_counter_min_clock_tick_periodic;
    uint8_t page_prot_and_oem_attr;
} __packed;
