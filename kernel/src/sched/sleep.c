/*
 * kernel/src/sched/sleep.c
 * © suhas pai
 */

#include "sched/alarm.h"
#include "sleep.h"

__optimize(3) void sched_sleep_us(const usec_t usecs) {
    struct alarm alarm = ALARM_INIT(alarm, usecs);
    alarm_post(&alarm, /*await=*/true);
}
