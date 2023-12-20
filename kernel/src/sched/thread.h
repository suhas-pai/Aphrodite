/*
 * kernel/src/sched/thread.h
 * © suhas pai
 */

#pragma once

#include "cpu/info.h"

#include "info.h"
#include "process.h"

struct thread {
    struct process *process;
    struct cpu_info *cpu;

    struct sched_thread_info sched_info;
};

extern struct thread kernel_main_thread;