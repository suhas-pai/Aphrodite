/*
 * kernel/include/acpi/madt.h
 * © suhas pai
 */

#pragma once
#include "structs.h"

void madt_init(const struct os_acpi_madt *madt);
