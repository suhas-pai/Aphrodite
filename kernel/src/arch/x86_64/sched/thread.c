/*
 * kernel/src/arch/x86_64/sched/thread.c
 * © suhas pai
 */

#include "asm/fsgsbase.h"
#include "asm/xsave.h"
#include "lib/assert.h"

#include "sched/thread.h"

__debug_optimize(3) struct thread *current_thread() {
    return (struct thread *)gsbase_read();
}

__debug_optimize(3) void sched_set_current_thread(struct thread *const thread) {
    gsbase_write((uint64_t)thread);
    msr_write(IA32_MSR_KERNEL_GS_BASE, (uint64_t)thread);
}

extern __noreturn void thread_spinup(const struct thread_context *context);

void
sched_switch_to(struct thread *const prev,
                struct thread *const next,
                struct thread_context *const prev_context)
{
    if (prev->process == &kernel_process) {
        //xsave_supervisor_into(&prev->arch_info.avx_state);
    } else {
        verify_not_reached();
        xsave_user_into(&prev->arch_info.avx_state);
    }

    if (next->process == &kernel_process) {
        //xrstor_supervisor_from(&next->arch_info.avx_state);
    } else {
        verify_not_reached();
        xrstor_user_from(&next->arch_info.avx_state);
    }

    // Don't overwrite context of idle threads
    if (prev->cpu == NULL || prev != prev->cpu->idle_thread) {
        prev->context = *prev_context;
    }

    thread_context_verify(next->process, &next->context);
    thread_spinup(&next->context);

    verify_not_reached();
}
