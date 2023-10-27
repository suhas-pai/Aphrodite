/*
 * kernel/acpi/fadt.h
 * © suhas pai
 */

#pragma once
#include "acpi/structs.h"

void fadt_init(const struct acpi_fadt *fadt);