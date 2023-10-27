/*
 * kernel/arch/x86_64/dev/time/hpet.h
 * © suhas pai
 */

#pragma once
#include "acpi/extra_structs.h"

void hpet_init(const struct acpi_hpet *hpet);
uint64_t hpet_get_femto();