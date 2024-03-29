/*
 * kernel/src/sched/alarm.h
 * © suhas pai
 */

#pragma once

#include "lib/time.h"
#include "sched/thread.h"

struct thread;
struct alarm {
    struct list list;
    struct thread *listener;

    _Atomic bool active;
    usec_t remaining;
};

#define ALARM_INIT(name, time) \
    ((struct alarm){ \
        .list = LIST_INIT(name.list), \
        .listener = current_thread(), \
        .remaining = (time) \
    })

struct list *alarm_get_list_locked(int *flag_out);
void alarm_list_unlock(const int flag);

void alarm_post(struct alarm *alarm, bool await);
void alarm_clear(struct alarm *alarm);

bool alarm_cleared(const struct alarm *alarm);
