/*
 * kernel/src/cpu/mutex.h
 * © suhas pai
 */

#pragma once

#include "lib/list.h"
#include "lib/time.h"

struct mutex {
    _Atomic(uintptr_t) flags;
    struct list waiters;
};

#define MUTEX_INIT(name) \
    ((struct mutex){ \
        .locked = false, \
        .waiters = LIST_INIT(name.waiters) \
    })

enum mutex_flags : uint64_t {
    __MUTEX_FLAGS_OWNER_MASK = UINT64_MAX << 2,

    // Only one thread can be contending for the lock at a time. The contender
    // thread has full access to the lock's data structures, while other threads
    // have to wait for the contender to finish.

    __MUTEX_FLAGS_CONTENDED = 1 << 0,

    // As the contender thread has full access to the lock's data structures,
    // unlock,

    __MUTEX_FLAGS_HAS_WAITER = 1 << 1,
    __MUTEX_FLAGS_IN_CONTENTION =
        __MUTEX_FLAGS_CONTENDED | __MUTEX_FLAGS_HAS_WAITER
};

void mutex_init(struct mutex *mutex);

void mutex_lock(struct mutex *mutex);
void mutex_unlock(struct mutex *mutex);

bool mutex_try_lock(struct mutex *mutex);
bool mutex_lock_with_timeout(struct mutex *mutex, usec_t timeout);
