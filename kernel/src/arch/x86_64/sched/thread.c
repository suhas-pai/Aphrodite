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
}

void sched_prepare_thread(struct thread *const thread) {
    (void)thread;
}

__optimize(3)
__noreturn static void thread_spinup(struct thread_context *const context) {
    asm volatile (
        "mov %0, %%rsp\n"
        "pop %%r15\n"
        "pop %%r14\n"
        "pop %%r13\n"
        "pop %%r12\n"
        "pop %%r11\n"
        "pop %%r10\n"
        "pop %%r9\n"
        "pop %%r8\n"
        "pop %%rbp\n"
        "pop %%rdi\n"
        "pop %%rsi\n"
        "pop %%rdx\n"
        "pop %%rcx\n"
        "pop %%rax\n"
        "pop %%rbx\n"
        "pop %%rax\n"
        "swapgs\n"
        "iretq\n"
        :
        : "rm" (context)
        : "memory"
    );

    verify_not_reached();
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

    if (from_irq) {
        thread_spinup(&next->context);
    }

    verify_not_reached();
}
