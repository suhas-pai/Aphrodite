/*
 * kernel/src/sched/thread.h
 * © suhas pai
 */

#pragma once

#include "asm/context.h"
#include "cpu/info.h"
#include "sched/arch.h"

#include "info.h"
#include "process.h"

struct thread {
    struct process *process;
    struct cpu_info *cpu;

    _Atomic uint16_t preemption_disabled;

    bool signal_enqueued : 1;
    int64_t event_index;

    struct thread_context context;
    struct thread_arch_info arch_info;

    struct sched_thread_info sched_info;
};

#define with_preempt_disabled(block) \
    do { \
        preempt_disable(); \
        block; \
        preempt_enable(); \
    } while (false)

extern struct thread kernel_main_thread;

void
sched_thread_init(struct thread *thread,
                  struct process *process,
                  struct cpu_info *cpu,
                  const void *entry);

struct thread *current_thread();

bool thread_runnable(const struct thread *thread);
bool thread_enqueued(const struct thread *thread);
bool thread_running(const struct thread *thread);

bool preemption_enabled();

void preempt_disable();
void preempt_enable();
