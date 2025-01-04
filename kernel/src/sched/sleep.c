/*
 * kernel/src/sched/sleep.c
 * © suhas pai
 */

#include "sched/alarm.h"
#include "sched/sleep.h"

__debug_optimize(3) void sched_sleep_us(const usec_t usecs) {
    struct alarm alarm;

    alarm_create_for_self(&alarm, usecs);
    alarm_post(&alarm, /*await=*/true);
}
