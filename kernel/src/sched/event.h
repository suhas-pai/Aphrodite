/*
 * kernel/src/sched/event.h
 * © suhas pai
 */

#pragma once

#include "cpu/spinlock.h"
#include "sched/scheduler.h"

struct await_result {
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
        .result_u64 = 0, \
    })
#define AWAIT_RESULT_BOOL(value) \
    ((struct await_result){ \
        .result_bool = (value), \
    })
#define AWAIT_RESULT_U64(value) \
    ((struct await_result){ \
        .result_u64 = (value), \
    })
#define AWAIT_RESULT_U32(value) \
    ((struct await_result){ \
        .result_u32 = (value), \
    })
#define AWAIT_RESULT_U16(value) \
    ((struct await_result){ \
        .result_u16 = (value), \
    })
#define AWAIT_RESULT_U8(value) \
    ((struct await_result){ \
        .result_u8 = (value), \
    })
#define AWAIT_RESULT_CHAR(value) \
    ((struct await_result){ \
        .result_char = (value), \
    })
#define AWAIT_RESULT_PTR(value) \
    ((struct await_result){ \
        .result_ptr = (value), \
    })

struct event {
    struct spinlock lock;
    struct array listeners;

    uint32_t pending;
};

#define EVENT_INIT() \
    ((struct event){ \
        .lock = SPINLOCK_INIT(), \
        .listeners = ARRAY_INIT(sizeof(struct event_listener)), \
        .pending = 0, \
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
events_await(struct event *const *events,
             uint32_t events_count,
             bool block,
             bool drop_after_recv);

void event_trigger(struct event *event, bool drop_if_no_listeners);
