/*
 * kernel/src/arch/aarch64/acpi/gtdt.h
 * © suhas pai
 */

#pragma once
#include "acpi/structs.h"

void gtdt_init(const struct acpi_gtdt *gtdt);
