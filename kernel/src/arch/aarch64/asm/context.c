/*
 * kernel/src/aarch64/asm/context.c
 * © suhas pai
 */

#include "sched/process.h"
#include "context.h"

void
thread_context_verify(const struct process *const process,
                      const struct thread_context *const context)
{
    if (process == &kernel_process) {
        assert(context->elr_el1 > KERNEL_BASE);
    }
}
