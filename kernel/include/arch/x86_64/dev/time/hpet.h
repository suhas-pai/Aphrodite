/*
 * kernel/include/arch/x86_64/dev/time/hpet.h
 * © suhas pai
 */

#pragma once

#include <lib/time.h>
#include "acpi/extra_structs.h"

void hpet_init(const struct os_acpi_hpet *hpet);
void hpet_oneshot_fsec(fsec_t fsec);

bool hpet_initialized();
uint64_t hpet_get_femto();
