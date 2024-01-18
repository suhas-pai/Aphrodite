/*
 * kernel/src/arch/riscv64/sched/timer.c
 * © suhas pai
 */

#include "dev/time/stime.h"
#include "lib/time.h"

__optimize(3) void sched_timer_oneshot(const usec_t usec) {
    stimer_oneshot(usec);
}

__optimize(3) void sched_timer_stop() {
    stimer_stop();
}

void sched_irq_eoi() {

}