/*
 * kernel/src/sched/alarm.c
 * © suhas pai
 */

#include "alarm.h"

__hidden struct list g_sched_alarm_list = LIST_INIT(g_sched_alarm_list);