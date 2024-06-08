/*
 * kernel/src/arch/aarch64/sched/arch.c
 * © suhas pai
 */

#include "asm/context.h"
#include "mm/page_alloc.h"
#include "sched/thread.h"

#define KERNEL_STACK_SIZE_ORDER 2
#define USER_STACK_SIZE_ORDER 2

void sched_process_arch_info_init(struct process *const process) {
    (void)process;
}

void
sched_thread_arch_info_init(struct thread *const thread,
                            const void *const entry)
{
    (void)thread;
    (void)entry;

    thread->arch_info.kernel_stack =
        alloc_pages(PAGE_STATE_KERNEL_STACK,
                    __ALLOC_ZERO,
                    KERNEL_STACK_SIZE_ORDER);

    void *const stack = page_to_virt(thread->arch_info.kernel_stack);
    thread->context =
        THREAD_CONTEXT_INIT(stack,
                            /*stack_size=*/PAGE_SIZE << KERNEL_STACK_SIZE_ORDER,
                            entry,
                            /*arg=*/NULL);
}