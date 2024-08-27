/*
 * kernel/src/sched/sleep.c
 * © suhas pai
 */

#include "mm/kmalloc.h"
#include "sched/alarm.h"

#include "sleep.h"

__debug_optimize(3) void sched_sleep_us(const usec_t usecs) {
    struct alarm *const alarm = alarm_create(usecs);
    alarm_post(alarm, /*await=*/true);
}
