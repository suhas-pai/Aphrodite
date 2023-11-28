/*
 * kernel/src/arch/aarch64/dev/time/time.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "sys/boot.h"

static uint64_t g_frequency = 0;

uint64_t nsec_since_boot() {
    return (uint64_t)boot_get_time();
}

void arch_init_time() {
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(g_frequency));
    printk(LOGLEVEL_INFO,
           "time: frequency is %" PRIu64 " ticks/ms\n",
           g_frequency);
}