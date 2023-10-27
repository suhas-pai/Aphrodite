/*
 * kernel/arch/aarch64/cpu.h
 * © suhas pai
 */

#pragma once

#include "acpi/structs.h"
#include "mm/pagemap.h"

struct pagemap;
struct cpu_info {
    struct pagemap *pagemap;

    struct list pagemap_node;
    struct list cpu_list;

    uint64_t spur_int_count;

    uint32_t cpu_interface_number;
    uint32_t acpi_processor_id;

    uint16_t spe_overflow_interrupt;
    uint64_t mpidr;

    struct mmio_region *cpu_interface_region;
    volatile struct gic_cpu_interface *interface;
};

extern struct list g_cpu_list;
void cpu_init();

void
cpu_add_gic_interface(const struct acpi_madt_entry_gic_cpu_interface *intr);

const struct cpu_info *get_base_cpu_info();
const struct cpu_info *get_cpu_info();
struct cpu_info *get_cpu_info_mut();