/*
 * kernel/src/arch/x86_64/dev/time/time.c
 * © suhas pai
 */

#include "dev/time/hpet.h"

#include "lib/time.h"
#include "sys/boot.h"

__debug_optimize(3) nsec_t nsec_since_boot() {
    return seconds_to_nano((sec_t)boot_get_time()) +
            femto_to_nano(hpet_get_femto());
}

void arch_init_time() {
    // No longer used with multi-processor setups and LAPIC
    // pit_init(PIT_DEFAULT_FLAGS, PIT_GRANULARITY_5_MS);
}