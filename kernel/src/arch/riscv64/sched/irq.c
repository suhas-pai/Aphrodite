/*
 * kernel/src/arch/riscv64/sched/irq.c
 * © suhas pai
 */

#include "lib/macros.h"
#include "sys/isr.h"

__hidden isr_vector_t g_sched_vector = 0;

void sched_init_irq() {

}

__optimize(3) isr_vector_t sched_get_isr_vector() {
    return g_sched_vector;
}