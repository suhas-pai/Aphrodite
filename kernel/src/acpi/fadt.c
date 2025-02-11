/*
 * kernel/src/acpi/fadt.c
 * © suhas pai
 */

#include "acpi/fadt.h"
#include "dev/printk.h"

#if defined(__aarch64__)
    #include "dev/psci.h"
#endif /* defined(__aarch64__) */

void fadt_init(const struct os_acpi_fadt *const fadt) {
    printk(LOGLEVEL_INFO,
           "fadt: version %" PRIu8 ".%" PRIu8 "\n",
           fadt->sdt.rev,
           fadt->minor_version);

#if defined(__x86_64__)
    printk(LOGLEVEL_INFO,
           "fadt: flags: 0x%" PRIx32 "\n",
           fadt->iapc_boot_arch_flags);

    if (fadt->iapc_boot_arch_flags &
            __OS_ACPI_FADT_IAPC_BOOT_MSI_NOT_SUPPORTED)
    {
        printk(LOGLEVEL_WARN, "fadt: msi is not supported\n");
    }

    if ((fadt->iapc_boot_arch_flags & __OS_ACPI_FADT_IAPC_BOOT_8042) == 0) {
        printk(LOGLEVEL_WARN, "fadt: ps2 devices are not supported\n");
    }

    assert_msg(fadt->century != 0, "fadt: rtc century reg not supported");
#elif defined(__aarch64__)
    printk(LOGLEVEL_INFO,
           "fadt: flags: 0x%" PRIx32 "\n",
           fadt->arm_boot_arch_flags);

    if (fadt->arm_boot_arch_flags & __OS_ACPI_FADT_ARM_BOOT_PSCI_COMPLIANT) {
        printk(LOGLEVEL_INFO, "fadt: system is psci compliant\n");
        const bool use_hvc =
            fadt->arm_boot_arch_flags & __OS_ACPI_FADT_ARM_BOOT_PSCI_USE_HVC;

        if (use_hvc) {
            printk(LOGLEVEL_INFO, "fadt: psci needs hvc\n");
        }

    #ifndef CONFIG_UACPI
        psci_init_from_acpi(use_hvc);
    #endif /* !defined(CONFIG_UACPI) */
    }
#endif /* defined(__x86_64__) */
}