/*
 * kernel/src/sched/event.c
 * © suhas pai
 */

#include "asm/irqs.h"
#include "sched/thread.h"

#include "event.h"

__optimize(3) static inline void
lock_events(struct event *const *const events, const uint32_t event_count) {
    for (uint32_t i = 0; i != event_count; i++) {
        spin_acquire(&events[i]->lock);
    }
}

__optimize(3) static inline void
unlock_events(struct event *const *const events, const uint32_t event_count) {
    for (uint32_t i = 0; i != event_count; i++) {
        spin_release(&events[i]->lock);
    }
}

__optimize(3) static inline void
pop_result(struct event *const event, struct await_result *const result_out) {
    *result_out = *(struct await_result *)array_front(event->pending);
    array_remove_index(&event->pending, /*index=*/0);
}

__optimize(3) static inline int64_t
find_pending(struct event *const *const events,
             const uint32_t event_count,
             struct await_result *const result_out)
{
    for (uint32_t i = 0; i != event_count; i++) {
        if (!array_empty(events[i]->pending)) {
            pop_result(events[i], result_out);
            return i;
        }
    }

    return -1;
}

__optimize(3) static inline void
add_listeners_to_events(struct event *const *const events,
                        const uint32_t event_count,
                        struct thread *const thread)
{
    for (uint32_t i = 0; i != event_count; i++) {
        const struct event_listener listener =
            EVENT_LISTENER_INIT(thread, array_item_count(events[i]->listeners));

        assert(array_append(&events[i]->listeners, &listener));
        assert(array_append(&thread->events_hearing, &listener));
    }
}

__optimize(3) static inline void
remove_thread_from_listeners(struct event *const event,
                             struct thread *const thread)
{
    thread->event_index = -1;

    const uint32_t item_count = array_item_count(event->listeners);
    for (uint32_t index = 0; index != item_count; index++) {
        const struct event_listener *const listener =
            array_at(event->listeners, index);

        if (listener->waiter == thread) {
            array_remove_index(&event->listeners, index);
            return;
        }
    }
}

int64_t
events_await(struct event *const *const events,
             const uint32_t events_count,
             const bool block,
             struct await_result *const result_out)
{
    int flag = disable_interrupts_if_not();
    lock_events(events, events_count);

    const int64_t index = find_pending(events, events_count, result_out);
    if (index != -1) {
        unlock_events(events, events_count);
        remove_thread_from_listeners(events[index], current_thread());
        enable_interrupts_if_flag(flag);

        return index;
    }

    if (!block) {
        unlock_events(events, events_count);
        enable_interrupts_if_flag(flag);

        return -1;
    }

    struct thread *const thread = current_thread();

    add_listeners_to_events(events, events_count, thread);
    sched_dequeue_thread(thread);

    unlock_events(events, events_count);
    sched_yield(/*noreturn=*/false);

    flag = disable_interrupts_if_not();

    const int64_t ret = thread->event_index;
    thread->event_index = -1;

    if (ret != -1) {
        pop_result(events[ret], result_out);
    }

    enable_interrupts_if_flag(flag);
    return ret;
}

void
event_trigger(struct event *const event,
              const struct await_result *const result,
              const bool drop_if_no_listeners)
{
    const int flag = spin_acquire_with_irq(&event->lock);
    if (!array_empty(event->listeners)) {
        array_foreach(&event->listeners, const struct event_listener, listener)
        {
            listener->waiter->event_index = listener->listeners_index;
            sched_enqueue_thread(listener->waiter);
        }

        array_append(&event->pending, result);
    } else {
        if (!drop_if_no_listeners) {
            array_append(&event->pending, result);
        }
    }

    spin_release_with_irq(&event->lock, flag);
}
