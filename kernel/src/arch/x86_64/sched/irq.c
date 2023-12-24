/*
 * kernel/src/sched/irq.c
 * © suhas pai
 */

#include "cpu/isr.h"

__hidden isr_vector_t g_sched_vector = 0;

void sched_init_irq() {
    g_sched_vector = isr_alloc_vector();
}