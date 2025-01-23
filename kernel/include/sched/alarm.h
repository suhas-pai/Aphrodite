/*
 * kernel/include/sched/alarm.h
 * © suhas pai
 */

#pragma once

#include <lib/list.h>
#include <lib/time.h>

struct alarm;
typedef void (*alarm_callback)(struct alarm *alarm, void *ctx);

struct thread;
struct alarm {
    struct list list;
    const struct cpu_info *cpu;

    alarm_callback callback;
    void *ctx;

    _Atomic bool posted;
    _Atomic bool triggered;

    usec_t remaining;
};

void
alarm_create(struct alarm *alarm,
             usec_t time,
             alarm_callback callback,
             void *ctx);

void alarm_create_for_self(struct alarm *alarm, usec_t time);

void alarm_post(struct alarm *alarm, bool await);
void alarm_await(struct alarm *alarm);
void alarm_clear(struct alarm *alarm);

bool alarm_triggered(const struct alarm *alarm);
