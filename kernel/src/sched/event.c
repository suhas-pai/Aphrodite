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

__optimize(3) static inline int64_t
find_pending(struct event *const *const events, const uint32_t event_count) {
    for (uint32_t i = 0; i != event_count; i++) {
        if (events[i]->pending != 0) {
            events[i]->pending--;
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
    }
}

__optimize(3) static inline void
remove_thread_from_listeners(struct event *const event,
                             struct thread *const thread)
{
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
             const bool drop_after_recv)
{
    int flag = disable_interrupts_if_not();
    lock_events(events, events_count);

    int64_t index = find_pending(events, events_count);
    if (index != -1) {
        unlock_events(events, events_count);
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
    sched_yield();

    flag = disable_interrupts_if_not();
    index = thread->event_index;

    thread->event_index = -1;
    if (drop_after_recv) {
        remove_thread_from_listeners(events[index], current_thread());
    }

    enable_interrupts_if_flag(flag);
    return index;
}

void event_trigger(struct event *const event, const bool drop_if_no_listeners) {
    const int flag = spin_acquire_with_irq(&event->lock);
    if (!array_empty(event->listeners)) {
        array_foreach(&event->listeners, const struct event_listener, listener)
        {
            listener->waiter->event_index = listener->listeners_index;
            sched_enqueue_thread(listener->waiter);
        }
    } else {
        if (!drop_if_no_listeners) {
            event->pending++;
        }
    }

    spin_release_with_irq(&event->lock, flag);
}
