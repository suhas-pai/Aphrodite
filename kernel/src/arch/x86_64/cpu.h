/*
 * kernel/arch/x86_64/cpu.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lib/list.h"
#include "mm/pagemap.h"

struct cpu_capabilities {
    bool supports_avx512 : 1;
    bool supports_x2apic : 1;
    bool supports_1gib_pages : 1;
};

struct pagemap;
struct cpu_info {
    uint32_t processor_id;
    uint32_t lapic_id;
    uint32_t lapic_timer_frequency;
    uint64_t timer_ticks;

    struct pagemap *pagemap;
    struct list pagemap_node;

    // Keep track of spurious interrupts for every lapic.
    uint64_t spur_int_count;
};

const struct cpu_info *get_base_cpu_info();
const struct cpu_info *get_cpu_info();
struct cpu_info *get_cpu_info_mut();

void cpu_init();
const struct cpu_capabilities *get_cpu_capabilities();