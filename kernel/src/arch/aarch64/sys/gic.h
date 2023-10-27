/*
 * kernel/arch/aarch64/sys/gic.h
 * © suhas pai
 */

#pragma once

#include "acpi/structs.h"
#include "lib/adt/array.h"
#include "mm/mmio.h"

#define GIC_SGI_INTERRUPT_START 0
#define GIC_SGI_INTERRUPT_END 15

#define GIC_PPI_INTERRUPT_START 16
#define GIC_PPI_INTERRUPT_END 31

#define GIC_SPI_INTERRUPT_START 32
#define GIC_SPI_INTERRUPT_END 1020

enum gic_version {
    GICv1 = 1,
    GICv2,
    GICv3,
    GICv4,
    GIC_VERSION_BACK = GICv4
};

struct gic_msi_frame {
    struct mmio_region *mmio;
    uint32_t id;

    uint16_t spi_base;
    uint16_t spi_count;

    bool use_msi_typer : 1;
};

struct gic_distributor {
    uint32_t hardware_id;
    uint32_t sys_vector_base;

    struct array msi_frame_list;
    enum gic_version version;

    uint8_t impl_cpu_count;
    uint8_t max_impl_lockable_spis;

    uint16_t interrupt_lines_count;
    bool supports_security_extensions : 1;
};

struct gic_cpu_interface;

void gic_cpu_init(volatile struct gic_cpu_interface *cpu);
void gic_dist_init(const struct acpi_madt_entry_gic_distributor *dist);

struct gic_msi_frame *
gic_dist_add_msi(const struct acpi_madt_entry_gic_msi_frame *frame);

const struct gic_distributor *get_gic_dist();