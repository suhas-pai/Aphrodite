/*
 * kernel/src/acpi/fadt.h
 * © suhas pai
 */

#pragma once
#include "structs.h"

void fadt_init(const struct acpi_fadt *fadt);