/*
 * kernel/src/sched/alarm.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "sched/alarm.h"
#include "sched/scheduler.h"

void
alarm_create(struct alarm *const alarm,
             const usec_t time,
             const alarm_callback callback,
             void *const ctx)
{
    list_init(&alarm->list);

    alarm->callback = callback;
    alarm->ctx = ctx;
    alarm->remaining = time;
    alarm->triggered = false;
}

static void alarm_self_callback(struct alarm *const alarm, void *const ctx) {
    (void)alarm;

    struct thread *const thread = (struct thread *)ctx;
    sched_wake(thread);
}

void alarm_create_for_self(struct alarm *const alarm, const usec_t time) {
    alarm_create(alarm, time, alarm_self_callback, current_thread());
}

__debug_optimize(3) static
int compare(const struct list *const theirs, const struct list *const ours) {
    const struct alarm *const our_alarm = parent_of(ours, struct alarm, list);
    const struct alarm *const their_alarm =
        parent_of(theirs, struct alarm, list);

    return twovar_cmp(their_alarm->remaining, our_alarm->remaining);
}

__debug_optimize(3)
void alarm_post(struct alarm *const alarm, const bool await) {
    with_preempt_disabled({
        alarm->cpu = this_cpu();
        list_add_inorder(&this_cpu_mut()->alarm_list, &alarm->list, compare);
    });

    atomic_store_explicit(&alarm->posted, true, memory_order_relaxed);
    if (await) {
        sched_await();
    }
}

__debug_optimize(3) void alarm_clear(struct alarm *const alarm) {
    list_remove(&alarm->list);
    atomic_store_explicit(&alarm->posted, false, memory_order_relaxed);
}

__debug_optimize(3) bool alarm_triggered(const struct alarm *const alarm) {
    return atomic_load_explicit(&alarm->triggered, memory_order_relaxed);
}