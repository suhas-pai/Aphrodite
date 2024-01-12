/*
 * kernel/src/arch/aarch64/cpu/info.h
 * © suhas pai
 */

#pragma once

#include "acpi/structs.h"
#include "cpu/cpu_info.h"

#include "sched/info.h"
#include "sched/thread.h"

#include "sys/gic.h"

struct pagemap;
struct cpu_info {
    struct pagemap *pagemap;

    struct list pagemap_node;
    struct list cpu_list;

    uint64_t spur_int_count;

    uint32_t cpu_interface_number;
    uint32_t acpi_processor_id;

    uint64_t mpidr;

    struct thread *idle_thread;
    struct gic_cpu_info gic_cpu;

    uint16_t spe_overflow_interrupt;
    bool is_active : 1;

    struct sched_percpu_info info;
};

extern struct list g_cpu_list;
void cpu_init();

void
cpu_add_gic_interface(const struct acpi_madt_entry_gic_cpu_interface *intr);