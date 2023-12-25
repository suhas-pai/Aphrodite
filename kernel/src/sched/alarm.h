/*
 * kernel/src/sched/alarm.h
 * © suhas pai
 */

#pragma once

#include "lib/list.h"
#include "time/clock.h"

struct sched_alarm {
    struct list list;
    struct clock *clock;
    struct timespec time;
};

#define SCHED_ALARM_INIT(name, clock_, spec) \
    ((struct sched_alarm){ \
        .list = LIST_INIT(name.list), \
        .clock = (clock_), \
        .time = (spec) \
    })
