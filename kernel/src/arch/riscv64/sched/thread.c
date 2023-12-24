/*
 * kernel/src/arch/riscv64/sched/thread.c
 * © suhas pai
 */

#include "sched/thread.h"
#include "lib/assert.h"

__optimize(3) struct thread *current_thread() {
    verify_not_reached();
}
