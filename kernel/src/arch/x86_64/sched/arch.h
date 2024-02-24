/*
 * kernel/src/arch/x86_64/sched/arch.h
 * © suhas pai
 */

#pragma once

struct process_arch_info {

};

struct process;
void sched_process_arch_info_init(struct process *process);

struct thread_arch_info {
    struct page *kernel_stack;
    struct irq_register_context *context;

    void *avx_state;
};

struct thread;
void sched_thread_arch_info_init(struct thread *thread, const void *entry);
