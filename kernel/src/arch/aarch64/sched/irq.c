/*
 * kernel/src/arch/aarch64/sched/irq.c
 * © suhas pai
 */

#include "sys/gic/v2.h"

#include "cpu/isr.h"
#include "sched/scheduler.h"

__hidden isr_vector_t g_sched_vector = 0;

__optimize(3) static void
sched_handle_irq(const uint64_t intr_no, struct thread_context *const frame) {
    (void)intr_no;
    sched_next(frame, /*from_irq=*/true);
}

__optimize(3) void sched_init_irq() {
    g_sched_vector = isr_alloc_vector(/*for_msi=*/false);
    assert(g_sched_vector != ISR_INVALID_VECTOR);

    isr_set_vector(g_sched_vector, sched_handle_irq, &ARCH_ISR_INFO_NONE());
}

__optimize(3) void sched_self_ipi() {

}

__optimize(3) isr_vector_t sched_get_isr_vector() {
    return g_sched_vector;
}

__optimize(3) void sched_send_ipi(const struct cpu_info *const cpu) {
    gicd_send_ipi(cpu->interface_number, sched_get_isr_vector());
}
