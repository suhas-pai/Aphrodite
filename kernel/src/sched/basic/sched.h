/*
 * kernel/src/sched/round_robin/sched.h
 * © suhas pai
 */

#pragma once

#include "lib/list.h"
#include "lib/time.h"

#define SCHED_BASIC_DEF_TIMESLICE_US (usec_t)5000

struct sched_process_info {

};

struct sched_thread_info {
    struct list list;

    usec_t timeslice : 32;
    usec_t remaining : 32;

    _Atomic bool awaiting;
    _Atomic bool enqueued;
};

struct sched_percpu_info {

};
