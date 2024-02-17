/*
 * kernel/src/arch/x86_64/sched/thread.c
 * © suhas pai
 */

#include "asm/context.h"
#include "asm/fsgsbase.h"

#include "asm/xsave.h"
#include "cpu/util.h"

#include "sched/process.h"
#include "sched/thread.h"

__optimize(3) struct thread *current_thread() {
    return (struct thread *)read_gsbase();
}

__optimize(3) void sched_set_current_thread(struct thread *const thread) {
    write_gsbase((uint64_t)thread);
}

void sched_prepare_thread(struct thread *const thread) {
    (void)thread;
}

void
sched_switch_to(struct thread *const prev,
                struct thread *const next,
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

    (void)from_irq;
}
