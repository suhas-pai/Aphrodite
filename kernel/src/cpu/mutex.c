/*
 * kernel/src/cpu/mutex.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "asm/irqs.h"
#include "asm/pause.h"

#include "cpu/mutex.h"
#include "lib/assert.h"

#include "sched/alarm.h"
#include "sched/scheduler.h"
#include "sched/thread.h"

struct mutex_waiter {
    struct list list;
    _Atomic(struct thread *) thread;
};

#define MUTEX_WAITER_INIT(name) \
    ((struct mutex_waiter){ \
        .list = LIST_INIT(name.list), \
        .thread = current_thread() \
    })

void mutex_init(struct mutex *const mutex) {
    list_init(&mutex->waiters);
    mutex->flags = 0;
}

bool
mutex_lock_fast(struct mutex *const mutex,
                struct thread *const thread,
                uintptr_t *const flags)
{
    if (atomic_compare_exchange_strong_explicit(&mutex->flags,
                                                flags,
                                                (uintptr_t)thread,
                                                memory_order_acq_rel,
                                                memory_order_acquire))
    {
        return true;
    }

    const uintptr_t owner = *flags & __MUTEX_FLAGS_OWNER_MASK;
    assert_msg(owner != (uintptr_t)thread,
               "Attempting to re-acquire mutex %p which is already owned by "
               "thread %p",
               mutex,
               thread);

    return false;
}

static inline bool
atomic_compare_exchange_flags(struct mutex *const mutex,
                              uintptr_t *const flags,
                              const uintptr_t desired)
{
    return
        atomic_compare_exchange_weak_explicit(&mutex->flags,
                                              /*expected=*/flags,
                                              desired,
                                              memory_order_acq_rel,
                                              memory_order_acquire);
}

// To acquire a mutex, we must first contend for the mutex. Contention is an
// exclusive state that grants us the right to run operations on behalf of the
// mutex. With contention, we can add ourselves to the wait-queue, while also
// taking care of other operations on behalf of our threads, such as waking up
// the next waiter for mutex_unlock().

enum contention_result : uint8_t {
    CONTENTION_SUCCESS,
    CONTENTION_FAILURE,
    CONTENTION_RETRY,
};

static enum contention_result
try_contention(struct mutex *const mutex,
               uintptr_t *const flags,
               const bool remove_has_waiter)
{
    // First check if other threads are contending, and proceed only if none
    // are.
    // This also avoids repeated calls to atomic-compare-exchange.

    if (*flags & __MUTEX_FLAGS_CONTENDED) {
        cpu_pause();
        *flags = atomic_load_explicit(&mutex->flags, memory_order_relaxed);

        return CONTENTION_RETRY;
    }

    // Signal our contention. We do a compare-and-exchange to ensure no
    // other thread has started contending in between the last atomic-load
    // and now.

    uintptr_t desired =
        (remove_has_waiter ? rm_mask(*flags, __MUTEX_FLAGS_HAS_WAITER) : *flags)
      | __MUTEX_FLAGS_CONTENDED;

    if (atomic_compare_exchange_flags(mutex, flags,desired)) {
        // As the sole contender, we can now attempt to acquire the lock.
        // If there is no owner, the lock can be acquired, which the
        // caller will do by atomically-storing either our thread pointer or
        // the thread pointer of the head of the waiter-queue.
        //
        // If we got here, but there is already an owner, then we'll have to
        // join the waiter-queue.

        if ((*flags & __MUTEX_FLAGS_OWNER_MASK) == 0) {
            return CONTENTION_SUCCESS;
        }

        return CONTENTION_FAILURE;
    }

    return CONTENTION_RETRY;
}

static bool
try_contention_and_hold(struct mutex *const mutex, uintptr_t *const flags) {
    while (true) {
        const enum contention_result result =
            try_contention(mutex, flags, /*remove_has_waiter=*/true);

        switch (result) {
            case CONTENTION_SUCCESS:
                return (*flags & __MUTEX_FLAGS_IN_CONTENTION) == 0;
            case CONTENTION_FAILURE:
                return false;
            case CONTENTION_RETRY:
                continue;
        }
    }

    verify_not_reached();
}

static void mutex_alarm_callback(struct alarm *const alarm, void *const ctx) {
    (void)alarm;

    struct mutex_waiter *const waiter = (struct mutex_waiter *)ctx;
    struct thread *const thread =
        atomic_load_explicit(&waiter->thread, memory_order_relaxed);

    if (thread != nullptr) {
        sched_wake(thread);
    }
}

bool
mutex_lock_slow(struct mutex *const mutex,
                struct mutex_waiter *const waiter,
                uintptr_t flags,
                const usec_t timeout)
{
    if (try_contention_and_hold(mutex, &flags)) {
        const uintptr_t new_flags = (uintptr_t)waiter->thread;
        atomic_store_explicit(&mutex->flags, new_flags, memory_order_relaxed);

        return true;
    }

    uintptr_t desired =
        rm_mask((flags | __MUTEX_FLAGS_HAS_WAITER), __MUTEX_FLAGS_CONTENDED);

    list_radd(&mutex->waiters, &waiter->list);
    if (!atomic_compare_exchange_flags(mutex, &flags, desired)) {
        // As we hold the contention bit, no other thread in mutex_lock() will
        // have modified mutex->flags. In this case, the owner thread has called
        // mutex_unlock().
        //
        // mutex_unlock() will set the HAS_WAITER bit in mutex->flags when the
        // it finds the contention bit has been set to signal to us, the
        // contender that it is our responsibility find and wake a waiter thread
        // on behalf of mutex_unlock().

        struct mutex_waiter *const head_waiter =
            list_head(&mutex->waiters, struct mutex_waiter, list);

        list_remove(&head_waiter->list);
        if (waiter == head_waiter) {
            // If our waiter is at the front of the queue, we can just become
            // the owner and exit.

            atomic_store_explicit(&mutex->flags,
                                  (uintptr_t)waiter,
                                  memory_order_release);
            return true;
        }

        sched_wake(head_waiter->thread);
        atomic_store_explicit(&mutex->flags,
                              (uintptr_t)head_waiter | __MUTEX_FLAGS_HAS_WAITER,
                              memory_order_release);
    }

    // At this point we have tried contending for the lock, only to find that
    // there are already other threads waiting for it.

    struct alarm alarm;
    if (timeout != 0) {
        alarm_create(&alarm, timeout, mutex_alarm_callback, waiter);
        alarm_post(&alarm, /*await=*/true);
    }

    // We've reached here in some combination of the following cases:
    //  1. We were able to acquire the lock.
    //  2. We were timed out.
    //
    // With (1), it's now our turn in the queue to acquire the lock, so we
    // proceed to contention and then hold the lock.
    // With (2), our timeout prevented us from waiting forever for the lock, and
    // we must now exit, but we should also check to see if its our turn to
    // acquire the lock
    //
    // One case or the other, or both, the following must happen:
    //  (1) We must remove ourselves from the wait-queue
    //  (2) We must try to acquire the lock if possible.

    const bool timed_out = timeout != 0 && alarm_triggered(&alarm);

    // Contend for the lock so we can remove ourselves from the wait-queue.
    while (true) {
        // As we haven't contended for the lock, there is a chance that another
        // thread, with contention, is waking up our thread. Handle that case
        // here.
        // The other thread will first atomically-set waiter->thread to nullptr,
        // before removing us from the wait-queue and waking our thread.
        // Wait for the mutex's flags to reflect our new ownership and exit.

        if (atomic_load_explicit(&waiter->thread, memory_order_relaxed) == nullptr)
        {
            uintptr_t mutex_owner =
                atomic_load_explicit(&mutex->flags, memory_order_relaxed) &
                __MUTEX_FLAGS_OWNER_MASK;

            while (mutex_owner != (uintptr_t)waiter) {
                mutex_owner =
                    atomic_load_explicit(&mutex->flags, memory_order_relaxed) &
                    __MUTEX_FLAGS_OWNER_MASK;
            }

            return true;
        }

        // If other threads aren't waking up, directly go for contending the
        // lock.

        flags = atomic_load_explicit(&mutex->flags, memory_order_relaxed);
        const enum contention_result result =
            try_contention(mutex, &flags, /*remove_has_waiter=*/false);

        switch (result) {
            case CONTENTION_SUCCESS:
                goto try_acquire;
            case CONTENTION_FAILURE:
            case CONTENTION_RETRY:
                continue;
        }

        cpu_pause();
    }

try_acquire:
    // Here we have contended for the lock, and it's our turn to acquire it.
    // We must remove ourselves from the wait-queue, and then try to acquire
    // the lock.

    // By this point in time, we should've already contended for the lock.

    assert(waiter->thread != nullptr);
    if (waiter->list.prev != &mutex->waiters) {
        assert_msg(timed_out,
                   "mutex_lock(%p): waiter is not at the front of waiter-queue, "
                   "and lock was not timed out",
                   mutex);
        return false;
    }

    list_remove(&waiter->list);
    flags = (uintptr_t)waiter->thread;

    if (!list_empty(&mutex->waiters)) {
        flags |= __MUTEX_FLAGS_HAS_WAITER;
    }

    atomic_store_explicit(&mutex->flags, flags, memory_order_release);
    return true;
}

void mutex_lock(struct mutex *const mutex) {
    struct mutex_waiter waiter = MUTEX_WAITER_INIT(waiter);
    uintptr_t flags = 0;

    if (mutex_lock_fast(mutex, waiter.thread, &flags)) {
        return;
    }

    assert_msg(intr_are_enabled(),
               "mutex_lock(%p) must be called with interrupts enabled",
               mutex);

    bool result = false;
    with_preempt_disabled({
        result = mutex_lock_slow(mutex, &waiter, flags, /*timeout=*/0);
    });

    assert_msg(result, "mutex_lock(%p) somehow failed", mutex);
}

static bool
mutex_unlock_fast(struct mutex *const mutex,
                  struct thread *const thread,
                  uintptr_t *const flags)
{
    *flags = (uintptr_t)thread;
    uintptr_t desired = 0;

    return atomic_compare_exchange_flags(mutex, flags, desired);
}

static
bool try_contention_for_unlock(struct mutex *const mutex, uintptr_t flags) {
    for (;;) {
        // If another lock is already contending, add has-waiter to signal our
        // presence.
        // Otherwise, try to contend for the lock.

        if (flags & __MUTEX_FLAGS_CONTENDED) {
            const uintptr_t desired = flags | __MUTEX_FLAGS_HAS_WAITER;
            if (atomic_compare_exchange_flags(mutex, &flags, desired)) {
                // We didn't contend for the lock, but we did add has-waiter, so
                // the contending thread is now responsible for waking up a
                // waiter. We can now exit as we have nothing left to do.

                return false;
            }
        } else {
            const uintptr_t desired =
                rm_mask(flags, __MUTEX_FLAGS_HAS_WAITER) |
                __MUTEX_FLAGS_CONTENDED;

            if (atomic_compare_exchange_flags(mutex, &flags, desired)) {
                return true;
            }
        }

        // If there are no waiters, and no one is in contention, attempt to
        // quickly unlock before another thread contends for the lock.

        if ((flags & __MUTEX_FLAGS_IN_CONTENTION) == 0) {
            if (atomic_compare_exchange_flags(mutex, &flags, /*desired=*/0)) {
                return true;
            }
        }

        cpu_pause();
    }

    return false;
}

static bool mutex_unlock_slow(struct mutex *const mutex) {
    assert_msg(!list_empty(&mutex->waiters),
               "mutex_unlock(%p) has no waiters, despite unlock attempt failed",
               mutex);

    struct mutex_waiter *const waiter =
        list_head(&mutex->waiters, struct mutex_waiter, list);

    struct thread *const thread = waiter->thread;
    atomic_store_explicit(&waiter->thread, nullptr, memory_order_relaxed);

    list_remove(&waiter->list);
    sched_wake(thread);

    uintptr_t desired = (uintptr_t)thread;
    if (!list_empty(&mutex->waiters)) {
        desired |= __MUTEX_FLAGS_HAS_WAITER;
    }

    atomic_store_explicit(&mutex->flags, desired, memory_order_relaxed);
    return true;
}

void mutex_unlock(struct mutex *const mutex) {
    struct thread *const thread = current_thread();
    uintptr_t flags = 0;

    if (mutex_unlock_fast(mutex, thread, &flags)) {
        return;
    }

    const uintptr_t owner = flags & __MUTEX_FLAGS_OWNER_MASK;
    assert_msg(owner != (uintptr_t)thread,
               "Attempting to re-acquire mutex %p which is already owned by "
               "thread %p",
               mutex,
               thread);

    // Assume we succeeded in unlocking the mutex, by default, as there is the
    // possibility in try_contention_for_unlock() that another thread has the
    // responsibility for waking up a waiter, and we don't have to do anything.

    bool result = true;
    with_preempt_disabled({
        if (try_contention_for_unlock(mutex, flags)) {
            result = mutex_unlock_slow(mutex);
        }
    });

    assert_msg(result, "mutex_unlock(%p) somehow failed", mutex);
}

bool mutex_try_lock(struct mutex *const mutex) {
    uintptr_t flags = 0;
    return mutex_lock_fast(mutex, current_thread(), &flags);
}
