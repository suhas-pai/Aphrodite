/*
 * kernel/arch/aarch64/dev/time/time.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/time.h"

#include "sys/boot.h"

uint64_t nsec_since_boot() {
    return (uint64_t)boot_get_time();
}

void arch_init_time() {}