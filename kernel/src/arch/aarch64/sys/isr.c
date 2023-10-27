/*
 * kernel/arch/aarch64/sys/isr.c
 * © suhas pai
 */

#include "asm/irq_context.h"
#include "cpu/isr.h"
#include "dev/printk.h"

extern void *ivt_el1;

void isr_init() {

}

void isr_install_vbar() {
    asm volatile("msr vbar_el1, %0" :: "r"(ivt_el1));
    printk(LOGLEVEL_INFO, "isr: installed vbar_el1\n");
}

isr_vector_t isr_alloc_vector() {
    panic("isr: isr_alloc_vector() called, not supported on aarch64");
}

void
isr_set_vector(const isr_vector_t vector,
               const isr_func_t handler,
               struct arch_isr_info *const info)
{
    (void)vector;
    (void)handler;
    (void)info;
}

void
isr_assign_irq_to_cpu(struct cpu_info *const cpu,
                      const uint8_t irq,
                      const isr_vector_t vector,
                      const bool masked)
{
    (void)cpu;
    (void)irq;
    (void)vector;
    (void)masked;
}

void handle_exception(irq_context_t *const context) {
    (void)context;
    printk(LOGLEVEL_INFO, "isr: got exception\n");
}

void handle_interrupt(irq_context_t *const context) {
    (void)context;
    printk(LOGLEVEL_INFO, "isr: got interrupt\n");
}