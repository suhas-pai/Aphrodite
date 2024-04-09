/*
 * kernel/src/arch/x86_64/sched/timer.c
 * © suhas pai
 */

#include "apic/lapic.h"
#include "lib/time.h"
#include "sched/irq.h"
#include "sys/irq.h"

__optimize(3) void sched_timer_oneshot(const usec_t usec) {
    lapic_timer_one_shot(usec, sched_get_isr_vector());
}

__optimize(3) void sched_timer_stop() {
    lapic_timer_stop();
}

__optimize(3) void sched_irq_eoi(const irq_number_t irq) {
    (void)irq;
    lapic_eoi();
}

__optimize(3) usec_t sched_timer_remaining() {
    return lapic_timer_remaining();
}
