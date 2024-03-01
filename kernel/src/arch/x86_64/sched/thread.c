/*
 * kernel/src/arch/x86_64/sched/thread.c
 * © suhas pai
 */

#include "asm/context.h"
#include "asm/fsgsbase.h"
#include "asm/xsave.h"

#include "sched/process.h"
#include "sched/thread.h"

__optimize(3) struct thread *current_thread() {
    return (struct thread *)read_gsbase();
}

__optimize(3) void sched_set_current_thread(struct thread *const thread) {
    write_gsbase((uint64_t)thread);
    msr_write(IA32_MSR_KERNEL_GS_BASE, (uint64_t)thread);
}

void sched_prepare_thread(struct thread *const thread) {
    (void)thread;
}

extern __noreturn void thread_spinup(const struct thread_context *context);

void
sched_switch_to(struct thread *const prev,
                struct thread *const next,
                struct thread_context *const prev_context,
                const bool from_irq)
{
    if (prev->process == &kernel_process) {
        //xsave_supervisor_into(&prev->arch_info.avx_state);
    } else {
        xsave_user_into(&prev->arch_info.avx_state);
    }

    if (next->process == &kernel_process) {
        //xrstor_supervisor_from(&next->arch_info.avx_state);
    } else {
        xrstor_user_from(&next->arch_info.avx_state);
    }

    prev->context = *prev_context;
    if (from_irq) {
        thread_spinup(&next->context);
    }

    verify_not_reached();
}
