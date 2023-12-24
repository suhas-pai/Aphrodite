/*
 * kernel/src/sched/sched.c
 * © suhas pai
 */

#include "scheduler.h"
#include "thread.h"
#include "timer.h"

void sched_next(struct scheduler *const sched) {
    (void)sched;

    struct thread *const thread = current_thread();
    if (thread->premption_disabled) {
        sched_irq_eoi();
        sched_timer_oneshot(thread->sched_info.timeslice);

        return;
    }
}