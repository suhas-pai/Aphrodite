/*
 * kernel/src/arch/aarch64/sched/arch.h
 * © suhas pai
 */

#pragma once

struct process_arch_info {

};

struct thread_arch_info {

};

struct process;
void sched_process_arch_info_init(struct process *proc);

struct thread;
void sched_thread_arch_info_init(struct thread *thread);
