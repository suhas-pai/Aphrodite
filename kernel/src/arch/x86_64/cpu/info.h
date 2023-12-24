/*
 * kernel/src/arch/x86_64/cpu/info.h
 * © suhas pai
 */

#pragma once
#include "cpu/cpu_info.h"

#include "mm/pagemap.h"
#include "sched/thread.h"

struct cpu_capabilities {
    bool supports_avx512 : 1;
    bool supports_x2apic : 1;
    bool supports_1gib_pages : 1;
    bool has_compacted_xsave : 1;

    uint16_t xsave_user_size;
    uint16_t xsave_supervisor_size;

    uint32_t xsave_user_features;
    uint32_t xsave_supervisor_features;
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
    struct thread *idle_thread;
    uint64_t spur_int_count;

    bool active : 1;
};

void cpu_init();
const struct cpu_capabilities *get_cpu_capabilities();