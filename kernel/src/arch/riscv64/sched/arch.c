/*
 * kernel/src/arch/riscv64/sched/arch.c
 * © suhas pai
 */

#include "sched/process.h"
#include "sched/thread.h"

void sched_process_arch_info_init(struct process *const proc) {
    (void)proc;
}

void
sched_thread_arch_info_init(struct thread *const thread,
                            const void *const entry)
{
    (void)entry;
    thread->arch_info.frame = STACK_FRAME_INIT();
}