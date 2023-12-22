/*
 * kernel/src/arch/x86_64/dev/time/time.c
 * © suhas pai
 */

#include "dev/time/hpet.h"
#include "lib/time.h"

#include "sys/boot.h"

nsec_t nsec_since_boot() {
    return (nsec_t)boot_get_time() + femto_to_nano(hpet_get_femto());
}

void arch_init_time() {}