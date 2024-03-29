/*
 * kernel/src/arch/aarch64/sched/timer.c
 * © suhas pai
 */

#include "lib/time.h"

void sched_timer_oneshot(const usec_t usec) {
    (void)usec;
    verify_not_reached();
}

void sched_timer_stop() {
    verify_not_reached();
}

void sched_irq_eoi() {
    verify_not_reached();
}

usec_t sched_timer_remaining() {
    verify_not_reached();
}