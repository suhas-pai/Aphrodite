/*
 * kernel/src/sched/event.c
 * © suhas pai
 */

#include "asm/irqs.h"
#include "sched/thread.h"

#include "event.h"

__optimize(3) static inline
void lock_events(struct event **const events, const uint32_t event_count) {
    for (uint32_t i = 0; i != event_count; i++) {
        spin_acquire(&events[i]->lock);
    }
}

__optimize(3) static inline
void unlock_events(struct event **const events, const uint32_t event_count) {
    for (uint32_t i = 0; i != event_count; i++) {
        spin_release(&events[i]->lock);
    }
}

__optimize(3) static inline
int64_t find_pending(struct event **const events, const uint32_t event_count) {
    for (uint32_t i = 0; i != event_count; i++) {
        if (events[i]->pending_count > 0) {
            events[i]->pending_count--;
            return i;
        }
    }

    return -1;
}

__optimize(3) static inline void
add_listeners_to_events(struct event **const events,
                        const uint32_t event_count,
                        struct thread *const thread)
{
    for (uint32_t i = 0; i != event_count; i++) {
        const struct event_listener listener =
            EVENT_LISTENER_INIT(thread, array_item_count(events[i]->listeners));

        assert(array_append(&events[i]->listeners, &thread));
        assert(array_append(&thread->events_hearing, &listener));
    }
}

int64_t
events_await(struct event **const events,
             const uint32_t event_count,
             const bool block)
{
    const bool flag = disable_all_irqs_if_not();
    lock_events(events, event_count);

    const int64_t index = find_pending(events, event_count);
    if (index != -1) {
        unlock_events(events, event_count);
        enable_all_irqs_if_flag(flag);

        return index;
    }

    if (!block) {
        unlock_events(events, event_count);
        enable_all_irqs_if_flag(flag);

        return -1;
    }

    struct thread *const thread = current_thread();
    add_listeners_to_events(events, event_count, thread);

    unlock_events(events, event_count);
    enable_all_irqs_if_flag(flag);

    return -1;
}

void event_trigger(struct event *const event, const bool drop_if_no_listeners) {
    const bool flag = disable_all_irqs_if_not();
    if (array_empty(event->listeners)) {
        if (!drop_if_no_listeners) {
            event->pending_count++;
        }

        enable_all_irqs_if_flag(flag);
        return;
    }

    array_foreach(&event->listeners, const struct thread, listener) {

    }

    enable_all_irqs_if_flag(flag);
}
