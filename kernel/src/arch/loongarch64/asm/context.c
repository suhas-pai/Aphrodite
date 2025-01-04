/*
 * kernel/src/loongarch64/asm/context.c
 * © suhas pai
 */

#include "asm/context.h"
#include "sched/process.h"

void
thread_context_verify(const struct process *const process,
                      const struct thread_context *const context)
{
    if (process == &kernel_process) {
        assert(context->ra > KERNEL_BASE);
    }
}
