/*
 * kernel/arch/x86_64/apic/structs.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "lib/macros.h"

struct lapic_aligned_value {
    _Alignas(16) volatile uint32_t value;
};

enum lapic_timer_div_config {
    LAPIC_TIMER_DIV_CONFIG_BY_2,
    LAPIC_TIMER_DIV_CONFIG_BY_4,
    LAPIC_TIMER_DIV_CONFIG_BY_8,
    LAPIC_TIMER_DIV_CONFIG_BY_16,
    LAPIC_TIMER_DIV_CONFIG_BY_32 = 0b1000,
    LAPIC_TIMER_DIV_CONFIG_BY_64,
    LAPIC_TIMER_DIV_CONFIG_BY_128,
    LAPIC_TIMER_DIV_CONFIG_BY_1,
};

struct lapic_registers {
    _Alignas(16) volatile uint32_t reserved_1[8];
    _Alignas(16) volatile uint32_t id;
    _Alignas(16) volatile const uint32_t version;
    _Alignas(16) volatile uint32_t reserved_2[16];
    _Alignas(16) volatile uint32_t task_priority;
    _Alignas(16) volatile const uint32_t arbitration_priority;
    _Alignas(16) volatile const uint32_t processor_priority;
    _Alignas(16) volatile uint32_t eoi;
    _Alignas(16) volatile const uint32_t register_read;
    _Alignas(16) volatile uint32_t logical_destination;
    _Alignas(16) volatile uint32_t destination_format;
    _Alignas(16) volatile uint32_t spur_int_vector;

    _Alignas(16) volatile const struct lapic_aligned_value in_service[8];
    _Alignas(16) volatile const struct lapic_aligned_value trigger_mode[8];

    _Alignas(16) volatile const struct lapic_aligned_value interrupt_request[8];

    _Alignas(16) volatile const uint32_t error_status;
    _Alignas(16) volatile uint32_t reserved_4[24];

    // Local Vector Table Corrected Machine Check Interrupt (CMCI)
    _Alignas(16) volatile uint32_t lvt_cmci[4];

    // Icr = Interrupt Command Register
    _Alignas(16) volatile struct lapic_aligned_value icr[2];

    // Local Vector Table Timer Interrupt (TIMER)
    _Alignas(16) volatile uint32_t lvt_timer;
    _Alignas(16) volatile uint32_t lvt_thermal;
    _Alignas(16) volatile uint32_t lvt_performance_monitoring_counter;
    _Alignas(16) volatile uint32_t lvt_lint0;
    _Alignas(16) volatile uint32_t lvt_lint1;
    _Alignas(16) volatile uint32_t lvt_error;

    _Alignas(16) volatile uint32_t timer_initial_count;
    _Alignas(16) volatile uint32_t timer_current_count;

    _Alignas(16) volatile uint32_t reserved_5[16];

    _Alignas(16) volatile enum lapic_timer_div_config timer_divide_config;
    _Alignas(16) volatile uint32_t reserved_6[3];
} __packed;

enum apic_lvt_delivery_mode {
    APIC_LVT_DELIVERY_MODE_FIXED,
    APIC_LVT_DELIVERY_MODE_LOWEST,
    APIC_LVT_DELIVERY_MODE_SMI,

    // 0b11 is reserved

    APIC_LVT_DELIVERY_MODE_NMI = 0b100,
    APIC_LVT_DELIVERY_MODE_INIT,
    APIC_LVT_DELIVERY_MODE_STARTUP,
    APIC_LVT_DELIVERY_MODE_EXTINT
};

enum apic_lvt_dest_mode {
    APIC_LVT_DEST_MODE_PHYSICAL,
    APIC_LVT_DEST_MODE_LOGICAL
};

enum apic_lvt_trigger_mode {
    APIC_LVT_TRIGGER_MODE_EDGE,
    APIC_LVT_TRIGGER_MODE_LEVEL
};
