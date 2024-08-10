/*
 * kernel/src/arch/riscv64/sched/timer.c
 * © suhas pai
 */

#include "dev/time/stime.h"

#include "cpu/isr.h"
#include "sched/thread.h"
#include "sys/irqdef.h"

__debug_optimize(3) void sched_timer_oneshot(const usec_t usec) {
    stimer_oneshot(usec);
}

__debug_optimize(3) void sched_timer_stop() {
    stimer_stop();
}

__debug_optimize(3) usec_t sched_timer_remaining() {
    preempt_disable();
    const usec_t remaining = stime_get() - this_cpu()->timer_start;
    preempt_enable();

    return remaining;
}

__debug_optimize(3) void sched_irq_eoi(const irq_number_t irq) {
    isr_eoi(irq);
}