/*
 * kernel/src/arch/x86_64/sched/thread.c
 * © suhas pai
 */

#include "asm/fsgsbase.h"
#include "cpu/util.h"
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

void sched_switch_to(struct thread *const prev, struct thread *const next) {
    (void)prev;
    (void)next;
}

__optimize(3) void sched_switch_to_idle() {
    cpu_idle();
}