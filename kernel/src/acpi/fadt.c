/*
 * kernel/acpi/fadt.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "fadt.h"

void fadt_init(const struct acpi_fadt *const fadt) {
    printk(LOGLEVEL_INFO,
           "fadt: version %" PRIu8 ".%" PRIu8 "\n",
           fadt->sdt.rev,
           fadt->fadt_minor_version);

#if defined(__x86_64__)
    printk(LOGLEVEL_INFO,
           "fadt: flags: 0x%" PRIx32 "\n",
           fadt->iapc_boot_arch_flags);

    if (fadt->iapc_boot_arch_flags & __ACPI_FADT_IAPC_BOOT_MSI_NOT_SUPPORTED) {
        printk(LOGLEVEL_WARN, "fadt: msi is not supported\n");
    }

    if ((fadt->iapc_boot_arch_flags & __ACPI_FADT_IAPC_BOOT_8042) == 0) {
        printk(LOGLEVEL_WARN, "fadt: ps2 devices are not supported\n");
    }

    assert_msg(fadt->century != 0,
               "fadt: rtc century register not supported");
#elif defined(__aarch64__)
    printk(LOGLEVEL_INFO,
           "fadt: flags: 0x%" PRIx32 "\n",
           fadt->arm_boot_arch_flags);

    if (fadt->arm_boot_arch_flags & __ACPI_FADT_ARM_BOOT_PSCI_COMPLIANT) {
        printk(LOGLEVEL_INFO, "fadt: system is psci compliant\n");
    }

    if (fadt->arm_boot_arch_flags & __ACPI_FADT_ARM_BOOT_PSCI_USE_HVC) {
        printk(LOGLEVEL_INFO, "fadt: psci needs hvc\n");
    }
#endif /* defined(__x86_64__) */
}