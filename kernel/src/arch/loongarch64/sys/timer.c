/*
 * kernel/src/arch/loongarch64/sys/time.c
 * © suhas pai
 */

#include "asm/timer.h"
#include "asm/csr.h"

#include "timer.h"

void timer_oneshot(const uint32_t seconds) {
    const uint32_t config =
        seconds << TIMER_CONFIG_INIT_VALUE_SHIFT | __TIMER_CONFIG_ENABLE;

    csr_write(tcfg, config);
}

void timer_stop() {
    csr_write(tcfg, 0);
}

uint32_t timer_remaining() {
    return csr_read(tval);
}
