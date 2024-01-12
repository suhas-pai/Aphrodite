/*
 * kernel/src/sched/round_robin/sched.h
 * © suhas pai
 */

#pragma once

#include "lib/list.h"
#include "lib/time.h"

#define SCHED_ROUND_ROBIN_DEF_TIMESLICE_US (usec_t)5000

struct sched_process_info {

};

enum sched_thread_info_state {
    SCHED_THREAD_INFO_STATE_RUNNING,
    SCHED_THREAD_INFO_STATE_RUNNABLE,
    SCHED_THREAD_INFO_STATE_READY,
    SCHED_THREAD_INFO_STATE_ASLEEP,
};

struct sched_thread_info {
    struct list list;

    enum sched_thread_info_state state : 2;
    usec_t timeslice : 32;
};

struct sched_percpu_info {
};
