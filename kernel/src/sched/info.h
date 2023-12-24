/*
 * kernel/src/sched/info.h
 * © suhas pai
 */

#pragma once

#include "lib/time.h"
#include "scheduler.h"

struct sched_process_info {

};

enum sched_thread_state {
    SCHED_THREAD_STATE_NONE,
    SCHED_THREAD_STATE_RUNNABLE,
    SCHED_THREAD_STATE_RUNNING
};

struct sched_thread_info {
    struct scheduler *scheduler;
    usec_t timeslice;
};

#define SCHED_PROCESS_INFO_INIT() \
    ((struct sched_process_info){})

#define SCHED_THREAD_INFO_INIT() \
    ((struct sched_thread_info){ .timeslice = 0 })
