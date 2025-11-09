/*
 * kernel/src/sched/event.c
 * © suhas pai
 */

#include "asm/irqs.h"

#include "sched/event.h"
#include "sched/scheduler.h"

__debug_optimize(3) static inline void
lock_events(struct event *const *const events, const uint32_t event_count) {
    arrptr_foreach(events, event_count, event) {
        spin_acquire(&(*event)->lock);
    }
}

__debug_optimize(3) static inline void
unlock_events(struct event *const *const events, const uint32_t event_count) {
    arrptr_foreach(events, event_count, event) {
        spin_release(&(*event)->lock);
    }
}

__debug_optimize(3) static inline int64_t
find_pending(struct event *const *const events, const uint32_t event_count) {
    uint32_t i = 0;
    arrptr_foreach(events, event_count, iter) {
        struct event *const event = *iter;
        if (event->pending != 0) {
            event->pending--;
            return i;
        }

        i++;
    }

    return -1;
}

__debug_optimize(3) static inline void
add_listeners_to_events(struct event *const *const events,
                        const uint32_t event_count,
                        struct thread *const thread)
{
    arrptr_foreach(events, event_count, event_ptr) {
        struct event *const event = *event_ptr;
        const struct event_listener listener =
            EVENT_LISTENER_INIT(thread, array_item_count(event->listeners));

        assert(array_add(&event->listeners, &listener));
    }
}

__debug_optimize(3) static inline void
remove_thread_from_listeners(struct event *const event,
                             struct thread *const thread)
{
    array_foreach_mut(&event->listeners, const struct event_listener, listener)
    {
        if (listener->waiter == thread) {
            array_remove_item(&event->listeners, listener);
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
    bool flag = intr_save();
    lock_events(events, events_count);

    int64_t index = find_pending(events, events_count);
    if (index != -1) {
        unlock_events(events, events_count);
        intr_restore(flag);

        return index;
    }

    if (!block) {
        unlock_events(events, events_count);
        intr_restore(flag);

        return -1;
    }

    struct thread *const thread = current_thread();

    add_listeners_to_events(events, events_count, thread);
    sched_dequeue_thread(current_thread());

    unlock_events(events, events_count);
    sched_yield();

    with_intr_disabled({
        index = thread->event_index;
        thread->event_index = -1;

        if (drop_after_recv) {
            remove_thread_from_listeners(events[index], thread);
        }
    });

    return index;
}

__debug_optimize(3)
void event_trigger(struct event *const event, const bool drop_if_no_listeners) {
    with_spinlock_intr_disabled(&event->lock, {
        if (!array_empty(event->listeners)) {
            array_foreach(&event->listeners,
                          const struct event_listener,
                          listener)
            {
                listener->waiter->event_index = listener->listeners_index;
                sched_wake(listener->waiter);
            }
        } else {
            if (!drop_if_no_listeners) {
                event->pending++;
            }
        }
    });
}
