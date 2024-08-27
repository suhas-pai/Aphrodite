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

struct alarm *alarm_create(usec_t time);

void alarm_post(struct alarm *alarm, bool await);
void alarm_clear(struct alarm *alarm);

bool alarm_cleared(const struct alarm *alarm);
