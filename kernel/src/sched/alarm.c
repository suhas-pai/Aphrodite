/*
 * kernel/src/sched/alarm.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "sched/scheduler.h"
#include "alarm.h"

static struct list g_list = LIST_INIT(g_list);
static struct spinlock g_lock = SPINLOCK_INIT();

__debug_optimize(3) struct list *alarm_get_list_locked(int *const flag_out) {
    *flag_out = spin_acquire_save_irq(&g_lock);
    return &g_list;
}

__debug_optimize(3) void alarm_list_unlock(const int flag) {
    spin_release_restore_irq(&g_lock, flag);
}

__debug_optimize(3)
static int compare(struct list *const theirs, struct list *const ours) {
    struct alarm *const their_alarm = container_of(theirs, struct alarm, list);
    struct alarm *const our_alarm = container_of(ours, struct alarm, list);

    return twovar_cmp(their_alarm->remaining, our_alarm->remaining);
}

__debug_optimize(3) void alarm_post(struct alarm *const alarm, const bool await) {
    const int flag = spin_acquire_save_irq(&g_lock);

    list_add_inorder(&g_list, &alarm->list, compare);
    atomic_store_explicit(&alarm->active, true, memory_order_relaxed);

    if (!await) {
        spin_release_restore_irq(&g_lock, flag);
        return;
    }

    struct thread *const thread = current_thread();

    sched_dequeue_thread(thread);
    spin_release_restore_irq(&g_lock, flag);

    sched_yield();
}

__debug_optimize(3) void alarm_clear(struct alarm *const alarm) {
    const int flag = spin_acquire_save_irq(&g_lock);

    list_remove(&alarm->list);
    atomic_store_explicit(&alarm->active, false, memory_order_relaxed);

    spin_release_restore_irq(&g_lock, flag);
}

__debug_optimize(3) bool alarm_cleared(const struct alarm *const alarm) {
    return atomic_load_explicit(&alarm->active, memory_order_relaxed);
}