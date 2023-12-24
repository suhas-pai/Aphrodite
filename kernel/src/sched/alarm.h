/*
 * kernel/src/sched/alarm.h
 * © suhas pai
 */

#pragma once

#include "lib/list.h"
#include "lib/time.h"

struct sched_alarm {
    struct list list;
    struct timespec time;
    struct clock *clock;
};