/*
 * kernel/arch/riscv64/dev/init.c
 * © suhas pai
 */

#include "acpi/api.h"
#include "acpi/rhct.h"

#include "dev/printk.h"
#include "dev/uart/8250.h"

#include "mm/mm_types.h"
#include "mm/mmio.h"

void arch_init_dev() {
    const struct acpi_rhct *const rhct =
        (const struct acpi_rhct *)acpi_lookup_sdt("RHCT");

    if (rhct != NULL) {
        acpi_rhct_init(rhct);
    }
}