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
    const struct os_acpi_mcfg *mcfg;
    #if defined(__aarch64__)
        const struct os_acpi_gtdt *gtdt;
    #endif /* defined(__aarch64__) */

    const struct os_acpi_pptt *pptt;
    const struct os_acpi_rsdt *rsdt;

    const struct os_acpi_rsdp *rsdp;
    const struct os_acpi_madt *madt;
    const struct os_acpi_fadt *fadt;

    #if defined(__aarch64__)
        // Array of struct acpi_msi_frame
        struct array msi_frame_list;
    #endif /* defined(__aarch64__) */

    struct array iso_list;
    uint8_t nmi_lint : 1;
};

void acpi_init();

const struct os_acpi_sdt *acpi_lookup_sdt(const char signature[static 4]);
const struct acpi_info *get_acpi_info();

struct acpi_info *get_acpi_info_mut();
