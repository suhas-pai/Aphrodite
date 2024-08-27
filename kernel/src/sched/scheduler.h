/*
 * kernel/src/sched/scheduler.h
 * © suhas pai
 */

#pragma once

#include "sys/irqdef.h"
#include "thread.h"

void sched_init();
void sched_init_on_cpu(struct cpu_info *cpu);

void sched_algo_init();
void sched_algo_post_init();

struct thread_context;
void sched_next(irq_number_t irq, struct thread_context *context);

void sched_yield();

void
sched_save_restore_context(struct thread *prev,
                           struct thread *next,
                           struct thread_context *context);

void sched_enqueue_thread(struct thread *thread);
void sched_dequeue_thread(struct thread *thread);
