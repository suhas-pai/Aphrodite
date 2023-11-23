/*
 * kernel/src/arch/riscv64/dev/init.c
 * © suhas pai
 */

#include "acpi/api.h"
#include "acpi/rhct.h"

void arch_init_dev() {
    const struct acpi_rhct *const rhct =
        (const struct acpi_rhct *)acpi_lookup_sdt("RHCT");

    if (rhct != NULL) {
        acpi_rhct_init(rhct);
    }
}