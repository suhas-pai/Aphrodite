/*
 * kernel/src/arch/loongarch64/dev/time/time.c
 * © suhas pai
 */

#include "lib/time.h"
#include "sys/boot.h"

__optimize(3) nsec_t nsec_since_boot() {
    return seconds_to_nano((sec_t)boot_get_time());
}

void arch_init_time() {
}
