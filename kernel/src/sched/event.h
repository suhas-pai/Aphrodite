/*
 * kernel/src/sched/event.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/array.h"

#include "cpu/spinlock.h"
#include "sched/scheduler.h"

struct event {
    struct spinlock lock;
    struct array listeners;

    uint32_t pending_count;
};

#define EVENT_INIT() \
    ((struct event){ \
        .lock = SPINLOCK_INIT(), \
        .listeners = ARRAY_INIT(sizeof(struct event_listener)) \
    })

struct thread;
struct event_listener {
    struct thread *waiter;
    uint32_t listeners_index;
};

#define EVENT_LISTENER_INIT(thread, index) \
    ((struct event_listener){ \
        .waiter = (thread), \
        .listeners_index = (index) \
    })

int64_t
events_await(struct event *const *events, uint32_t events_count, bool block);

void event_trigger(struct event *event, bool drop_if_no_listeners);

struct await_result {
    struct event *event;
    union {
        uint64_t result_u64;
        uint32_t result_u32;
        uint16_t result_u16;
        uint8_t result_u8;

        bool result_bool;
        char result_char;

        void *result_ptr;
    };
};

#define AWAIT_RESULT_NULL() \
    ((struct await_result){ \
        .event = NULL, \
        .result_u64 = 0, \
    })
