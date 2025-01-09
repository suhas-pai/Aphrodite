/*
 * kernel/include/arch/riscv64/acpi/rhct.h
 * © suhas pai
 */

#pragma once
#include "extra_structs.h"

void acpi_rhct_init(const struct os_acpi_rhct *rhct);
