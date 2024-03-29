/*
 * kernel/src/arch/riscv64/sched/timer.c
 * © suhas pai
 */

#include "dev/time/stime.h"
#include "lib/time.h"
#include "sched/thread.h"

__optimize(3) void sched_timer_oneshot(const usec_t usec) {
    stimer_oneshot(usec);
}

__optimize(3) void sched_timer_stop() {
    stimer_stop();
}

usec_t sched_timer_remaining() {
    preempt_disable();
    const usec_t remaining = stime_get() - this_cpu()->timer_start;
    preempt_enable();

    return remaining;
}

void sched_irq_eoi() {

}