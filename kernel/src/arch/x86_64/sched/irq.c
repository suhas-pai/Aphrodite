/*
 * kernel/src/sched/irq.c
 * © suhas pai
 */

#include "cpu/isr.h"
#include "sched/scheduler.h"

__hidden isr_vector_t g_sched_vector = 0;

__optimize(3) static
void sched_handle_irq(const uint64_t int_no, irq_context_t *const frame) {
    (void)int_no;
    (void)frame;

    sched_next(/*from_irq=*/true);
}

__optimize(3) void sched_init_irq() {
    g_sched_vector = isr_alloc_vector();
    isr_set_vector(g_sched_vector, sched_handle_irq, &ARCH_ISR_INFO_NONE());
}