/*
 * kernel/src/arch/x86_64/sched/timer.c
 * © suhas pai
 */

#include "apic/lapic.h"
#include "lib/time.h"
#include "sched/sched.h"

__optimize(3) void sched_timer_oneshot(const usec_t usec) {
    lapic_timer_one_shot(usec, g_sched_vector);
}

__optimize(3) void sched_timer_stop() {
    lapic_timer_stop();
}

__optimize(3) void sched_irq_eoi() {
    lapic_eoi();
}

__optimize(3) usec_t sched_timer_remaining() {
    return lapic_timer_remaining();
}
