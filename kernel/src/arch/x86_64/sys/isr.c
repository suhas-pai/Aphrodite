/*
 * kernel/arch/x86_64/sys/isr.c
 * © suhas pai
 */

#include "apic/ioapic.h"

#include "cpu/isr.h"
#include "dev/printk.h"

#include "cpu.h"

static isr_func_t g_funcs[256] = {0};

static isr_vector_t g_free_vector = 0x21;
static isr_vector_t g_timer_vector = 0;
static isr_vector_t g_spur_vector = 0;

__optimize(3) isr_vector_t isr_alloc_vector() {
    assert(g_free_vector != 0xff);

    const isr_vector_t result = g_free_vector;
    g_free_vector++;

    return result;
}

__optimize(3) isr_vector_t isr_get_timer_vector() {
    return g_timer_vector;
}

__optimize(3) isr_vector_t isr_get_spur_vector() {
    return g_spur_vector;
}

__optimize(3)
void isr_handle_interrupt(const uint64_t vector, irq_context_t *const frame) {
    if (g_funcs[vector] != NULL) {
        g_funcs[vector](vector, frame);
    } else {
        printk(LOGLEVEL_INFO, "Got unhandled interrupt %" PRIu64 "\n", vector);
    }
}

__optimize(3)
static void spur_tick(const uint64_t int_no, irq_context_t *const frame) {
    (void)int_no;
    (void)frame;

    get_cpu_info_mut()->spur_int_count++;
}

void isr_init() {
    // Setup Timer
    g_timer_vector = isr_alloc_vector();

    // Setup Spurious Interrupt
    g_spur_vector = isr_alloc_vector();

    isr_set_vector(g_spur_vector, spur_tick, &ARCH_ISR_INFO_NONE());
    idt_register_exception_handlers();
}

__optimize(3) void
isr_set_vector(const isr_vector_t vector,
               const isr_func_t handler,
               struct arch_isr_info *const info)
{
    g_funcs[vector] = handler;
    idt_set_vector(vector, info->ist, IDT_DEFAULT_FLAGS);

    printk(LOGLEVEL_INFO,
           "isr: registered handler for vector %" PRIu8 "\n",
           vector);
}

__optimize(3) void
isr_assign_irq_to_cpu(struct cpu_info *const cpu,
                      const uint8_t irq,
                      const isr_vector_t vector,
                      const bool masked)
{
    ioapic_redirect_irq(cpu->lapic_id, irq, vector, masked);
}