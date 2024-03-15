/*
 * kernel/src/acpi/api.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/array.h"
#include "structs.h"

#if defined(__aarch64__)
    struct acpi_msi_frame {
        struct mmio_region *mmio;

        uint16_t spi_count;
        uint16_t spi_base;

        bool overriden_msi_typerr : 1;
    };
#endif /* defined(__aarch64__) */

struct apic_iso_info {
    uint8_t bus_src;
    uint8_t irq_src;
    uint8_t gsi;
    uint16_t flags;
};

struct acpi_info {
    const struct acpi_madt *madt;
    const struct acpi_fadt *fadt;
    const struct acpi_mcfg *mcfg;

#if defined(__aarch64__)
    const struct acpi_gtdt *gtdt;
#endif /* defined(__aarch64__) */

    const struct acpi_pptt *pptt;

    const struct acpi_rsdp *rsdp;
    const struct acpi_rsdt *rsdt;

#if defined(__aarch64__)
    // Array of struct acpi_msi_frame
    struct array msi_frame_list;
#endif /* defined(__aarch64__) */

    struct array iso_list;

    uint8_t nmi_lint : 1;
#if defined(__x86_64__)
    bool using_x2apic : 1;
#endif /* defined(__x86_64__) */
};

void acpi_init();
void acpi_parse_tables();

const struct acpi_sdt *acpi_lookup_sdt(const char signature[static 4]);

const struct acpi_info *get_acpi_info();
struct acpi_info *get_acpi_info_mut();
