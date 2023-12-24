/*
 * kernel/src/arch/aarch64/sched/thread.c
 * © suhas pai
 */

#include "sched/thread.h"

__optimize(3) struct thread *current_thread() {
    struct thread *thread = NULL;
    asm volatile ("mrs %0, tpidr_el1" : "=r"(thread));

    return thread;
}
