/*
 * kernel/arch/riscv64/dev/time/time.c
 * © suhas pai
 */

#include "sys/boot.h"

uint64_t nsec_since_boot() {
    return (uint64_t)boot_get_time();
}

void arch_init_time() {

}