/*
 * kernel/src/arch/loongarch64/sched/timer.c
 * © suhas pai
 */

#include "cpu/isr.h"
#include "lib/time.h"

#include "sys/irqdef.h"
#include "sys/timer.h"

__optimize(3) void sched_timer_oneshot(const usec_t usec) {
    timer_oneshot(usec);
}

__optimize(3) void sched_timer_stop() {
    timer_stop();
}

__optimize(3) usec_t sched_timer_remaining() {
    return timer_remaining();
}

__optimize(3) void sched_irq_eoi(const irq_number_t irq) {
    isr_eoi(irq);
}
