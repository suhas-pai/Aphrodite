/*
 * kernel/src/arch/x86_64/sched/arch.c
 * © suhas pai
 */

#include "asm/context.h"
#include "asm/xsave.h"

#include "mm/kmalloc.h"
#include "mm/page_alloc.h"

#include "sched/process.h"
#include "sched/thread.h"

#define KERNEL_STACK_SIZE_ORDER 2
#define USER_STACK_SIZE_ORDER 2

void sched_process_arch_info_init(struct process *const process) {
    (void)process;
}

__debug_optimize(3) void
sched_thread_arch_info_init(struct thread *const thread,
                            const void *const entry)
{
    if (thread->process == &kernel_process) {
        thread->arch_info.avx_state =
            kmalloc(sizeof(struct xsave_fx_legacy_regs));
    } else {
        thread->arch_info.avx_state = kmalloc(sizeof(struct xsave_avx_state));
    }

    thread->arch_info.kernel_stack =
        alloc_pages(PAGE_STATE_KERNEL_STACK,
                    __ALLOC_ZERO,
                    KERNEL_STACK_SIZE_ORDER);

    assert(thread->arch_info.avx_state != NULL);
    assert(thread->arch_info.kernel_stack != NULL);

    void *const stack = page_to_virt(thread->arch_info.kernel_stack);
    thread->context =
        THREAD_CONTEXT_INIT(thread->process,
                            stack,
                            /*stack_size=*/PAGE_SIZE << KERNEL_STACK_SIZE_ORDER,
                            entry,
                            /*arg=*/NULL);
}