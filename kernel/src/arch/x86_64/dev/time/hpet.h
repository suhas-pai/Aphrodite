/*
 * kernel/src/arch/x86_64/dev/time/hpet.h
 * © suhas pai
 */

#pragma once

#include "acpi/extra_structs.h"
#include "lib/time.h"

void hpet_init(const struct acpi_hpet *hpet);
void hpet_oneshot_fsec(fsec_t fsec);

uint64_t hpet_get_femto();
