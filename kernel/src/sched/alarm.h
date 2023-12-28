/*
 * kernel/src/sched/alarm.h
 * © suhas pai
 */

#pragma once

#include "lib/list.h"
#include "time/clock.h"

struct sched_alarm {
    struct list list;
    struct timespec time;

    const struct clock *clock;
};

struct sched_alarm *
sched_alarm_post(const struct clock *clock, struct timespec spec);
