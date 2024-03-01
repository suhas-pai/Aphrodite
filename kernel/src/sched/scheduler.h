/*
 * kernel/src/sched/scheduler.h
 * © suhas pai
 */

#pragma once
#include <stdbool.h>

void sched_init();
void sched_algo_init();

struct thread_context;

void sched_next(struct thread_context *context, bool from_irq);
void sched_yield(bool noreturn);

struct thread;
void sched_prepare_thread(struct thread *thread);

void
sched_switch_to(struct thread *prev,
                struct thread *next,
                struct thread_context *context,
                bool from_irq);

void sched_enqueue_thread(struct thread *thread);
void sched_dequeue_thread(struct thread *thread);
