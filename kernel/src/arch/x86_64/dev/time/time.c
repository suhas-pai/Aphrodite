/*
 * kernel/arch/x86_64/dev/time/time.c
 * © suhas pai
 */

#include "dev/time/hpet.h"
#include "lib/time.h"

#include "sys/boot.h"

uint64_t nsec_since_boot() {
    return (uint64_t)boot_get_time() + femto_to_nano(hpet_get_femto());
}

void arch_init_time() {
}