/*
 * kernel/src/arch/riscv64/sched/arch.h
 * © suhas pai
 */

#pragma once
#include "asm/stack_frame.h"

struct process_arch_info {

};

struct process;
void sched_process_arch_info_init(struct process *proc);

struct thread_arch_info {
    struct stack_frame frame;
};

struct thread;
void sched_thread_arch_info_init(struct thread *thread);
