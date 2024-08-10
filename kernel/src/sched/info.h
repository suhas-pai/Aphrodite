/*
 * kernel/src/sched/info.h
 * © suhas pai
 */

#pragma once

#if defined(SCHED_BASIC)
    #include "basic/sched.h"
#else
    #error "scheduler not set"
#endif /* defined(SCHED_BASIC) */

struct process;
struct thread;

void sched_process_algo_info_init(struct process *process);
void sched_thread_algo_info_init(struct thread *thread);
