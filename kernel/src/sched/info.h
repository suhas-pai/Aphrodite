/*
 * kernel/src/sched/info.h
 * © suhas pai
 */

#pragma once
#include "lib/time.h"

struct sched_process_info {

};

struct sched_thread_info {
    usec_t timeslice;
};

#define SCHED_PROCESS_INFO_INIT() \
    ((struct sched_process_info){})

#define SCHED_THREAD_INFO_INIT() \
    ((struct sched_thread_info){ .timeslice = 0 })