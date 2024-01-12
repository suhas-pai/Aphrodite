/*
 * kernel/src/arch/x86_64/sched/timer.c
 * © suhas pai
 */

#include "apic/lapic.h"
#include "lib/time.h"
#include "sched/sched.h"

void sched_timer_oneshot(const usec_t usec) {
    lapic_timer_one_shot(usec, g_sched_vector);
}

void sched_timer_stop() {
    lapic_timer_stop();
}

void sched_irq_eoi() {
    lapic_eoi();
}