/*
 * kernel/src/arch/x86_64/dev/time/time.c
 * © suhas pai
 */

#include <lib/time.h>
#include "dev/time/hpet.h"

#include "asm/pause.h"
#include "dev/pit.h"

#include "sys/boot.h"

__debug_optimize(3) nsec_t nsec_since_boot() {
    if (!hpet_initialized()) {
        return 0;
    }

    return seconds_to_nano((sec_t)boot_get_time()) +
           femto_to_nano(hpet_get_femto());
}

__debug_optimize(3) void stall_for_usec(const usec_t usec) {
    const usec_t current = hpet_get_femto();
    while (femto_to_micro(hpet_get_femto() - current) < usec) {
        cpu_pause();
    }
}

void arch_init_time_pre_acpi() {}
void arch_init_time_pre_dev_init() {
    // No longer used with multi-processor setups, but used to calculate LAPIC
    // frequency

    pit_init(PIT_DEFAULT_FLAGS, PIT_GRANULARITY_5_MS);
}

void arch_init_time() {}
