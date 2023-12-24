/*
 * kernel/src/sched/scheduler.h
 * © suhas pai
 */

#pragma once

enum scheduler_kind {
    SCHED_KIND_SIMPLE
};

struct scheduler {
    enum scheduler_kind kind;
};

void sched_init(struct scheduler *sched);
void sched_next(struct scheduler *sched);
void sched_yield();

struct thread;

void sched_enqueue_thread(struct thread *thread);
void sched_dequeue_thread(struct thread *thread);