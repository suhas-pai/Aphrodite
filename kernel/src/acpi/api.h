/*
 * kernel/acpi/init.h
 * © suhas pai
 */

#pragma once

#if __has_include("acpi/extra_structs.h")
    #include "acpi/extra_structs.h"
#endif /* __has_include("acpi/extra_structs.h") */

#include "acpi/structs.h"
#include "lib/adt/array.h"

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

    const struct acpi_rsdp *rsdp;
    const struct acpi_rsdt *rsdt;

#if defined(__aarch64__)
    // Array of struct acpi_msi_frame
    struct array msi_frame_list;
#endif /* defined(__x86_64__) */
    struct array iso_list;

    uint8_t nmi_lint : 1;
#if defined(__x86_64__)
    bool using_x2apic : 1;
#endif /* defined(__x86_64__) */
};

void acpi_init();

struct acpi_sdt *acpi_lookup_sdt(const char signature[static 4]);

const struct acpi_info *get_acpi_info();
struct acpi_info *get_acpi_info_mut();