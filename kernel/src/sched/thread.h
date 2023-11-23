/*
 * kernel/src/sched/thread.h
 * © suhas pai
 */

#pragma once

#include "info.h"
#include "process.h"

struct thread {
    struct process *process;
    struct sched_thread_info sched_info;
};