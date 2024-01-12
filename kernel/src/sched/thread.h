/*
 * kernel/src/sched/thread.h
 * © suhas pai
 */

#pragma once

#include "cpu/info.h"
#include "sched/arch.h"

#include "info.h"
#include "process.h"

struct thread {
    struct process *process;
    struct cpu_info *cpu;

    bool premption_disabled : 1;
    bool signal_enqueued : 1;

    struct array events_hearing;
    int64_t event_index;

    struct sched_thread_info sched_info;
    struct thread_arch_info arch_info;
};

extern struct thread kernel_main_thread;

void sched_thread_init(struct thread *thread, struct process *process);
struct thread *current_thread();

void prempt_disable();
void prempt_enable();
