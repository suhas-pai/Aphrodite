/*
 * kernel/src/sched/info.h
 * © suhas pai
 */

#pragma once

#if defined(SCHED_ROUND_ROBIN)
    #include "round_robin/sched.h"
#else
    #error "scheduler not set"
#endif /* defined(SCHED_ROUND_ROBIN) */

struct process;
struct thread;

void sched_process_algo_info_init(struct process *process);
void sched_thread_algo_info_init(struct thread *thread);
