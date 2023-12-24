/*
 * kernel/src/arch/x86_64/sched/thread.c
 * © suhas pai
 */

#include "asm/fsgsbase.h"
#include "sched/thread.h"

__optimize(3) struct thread *current_thread() {
    return (struct thread *)read_gsbase();
}

