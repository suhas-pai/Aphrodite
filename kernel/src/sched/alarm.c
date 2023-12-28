/*
 * kernel/src/sched/alarm.c
 * © suhas pai
 */

#include "mm/kmalloc.h"
#include "alarm.h"

__hidden struct list g_sched_alarm_list = LIST_INIT(g_sched_alarm_list);

struct sched_alarm *
sched_alarm_post(const struct clock *const clock, const struct timespec spec) {
    struct sched_alarm *const alarm = kmalloc(sizeof(*alarm));
    if (alarm == NULL) {
        return NULL;
    }

    list_init(&alarm->list);

    alarm->clock = clock;
    alarm->time = spec;

    struct sched_alarm *iter = NULL;
    list_foreach(iter, &g_sched_alarm_list, list) {
        if (timespec_compare(spec, iter->time) > 0) {
            list_add(&iter->list, &alarm->list);
        }
    }

    return alarm;
}
