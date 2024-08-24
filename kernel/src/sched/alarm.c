/*
 * kernel/src/sched/alarm.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "asm/irqs.h"
#include "sched/scheduler.h"

#include "alarm.h"

__debug_optimize(3)
static int compare(struct list *const theirs, struct list *const ours) {
    struct alarm *const their_alarm = container_of(theirs, struct alarm, list);
    struct alarm *const our_alarm = container_of(ours, struct alarm, list);

    return twovar_cmp(their_alarm->remaining, our_alarm->remaining);
}

__debug_optimize(3)
void alarm_post(struct alarm *const alarm, const bool await) {
    const int flag = disable_irqs_if_enabled();

    list_add_inorder(&this_cpu_mut()->alarm_list, &alarm->list, compare);
    enable_irqs_if_flag(flag);

    atomic_store_explicit(&alarm->active, true, memory_order_relaxed);
    if (!await) {
        return;
    }

    sched_yield();
}

__debug_optimize(3) void alarm_clear(struct alarm *const alarm) {
    list_remove(&alarm->list);
    atomic_store_explicit(&alarm->active, false, memory_order_relaxed);
}

__debug_optimize(3) bool alarm_cleared(const struct alarm *const alarm) {
    return atomic_load_explicit(&alarm->active, memory_order_relaxed);
}