/*
 * kernel/src/arch/x86_64/sys/isr.c
 * © suhas pai
 */

#include "apic/ioapic.h"

#include "apic/lapic.h"
#include "asm/msr.h"

#include "cpu/info.h"
#include "cpu/isr.h"

#include "dev/printk.h"
#include "lib/align.h"

static isr_func_t g_funcs[256] = {0};

static isr_vector_t g_free_vector = 0x21;
static isr_vector_t g_spur_vector = 0;
static isr_vector_t g_timer_vector = 0;

__optimize(3) isr_vector_t isr_alloc_vector(const bool for_msi) {
    (void)for_msi;
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

__optimize(3) void isr_mask_irq(const isr_vector_t irq) {
    (void)irq;
}

__optimize(3) void isr_unmask_irq(const isr_vector_t irq) {
    (void)irq;
}

extern void handle_exception(const uint64_t vector, irq_context_t *const frame);

__optimize(3)
void isr_handle_interrupt(const uint64_t vector, irq_context_t *const frame) {
    if (g_funcs[vector] != NULL) {
        g_funcs[vector](vector, frame);
    } else {
        if (vector < 0x20) {
            handle_exception(vector, frame);
        } else {
            printk(LOGLEVEL_INFO,
                   "Got unhandled interrupt %" PRIu64 "\n",
                   vector);
        }

        lapic_eoi();
    }
}

__optimize(3)
static void spur_tick(const uint64_t int_no, irq_context_t *const frame) {
    (void)int_no;
    (void)frame;

    this_cpu_mut()->spur_int_count++;
}

void isr_init() {
    // Setup Timer
    g_timer_vector = isr_alloc_vector(/*for_msi=*/false);

    // Setup Spurious Interrupt
    g_spur_vector = isr_alloc_vector(/*for_msi=*/false);

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

__optimize(3) void isr_eoi(const uint64_t int_no) {
    (void)int_no;
    lapic_eoi();
}

__optimize(3) enum isr_msi_support isr_get_msi_support() {
    return ISR_MSI_SUPPORT_MSIX;
}

__optimize(3) uint64_t
isr_get_msi_address(const struct cpu_info *const cpu, const isr_vector_t vector)
{
    (void)vector;
    return
        align_down(msr_read(IA32_MSR_APIC_BASE), PAGE_SIZE) |
        cpu->lapic_id << 12;
}

__optimize(3) uint64_t
isr_get_msix_address(const struct cpu_info *const cpu,
                     const isr_vector_t vector)
{
    (void)vector;
    return
        align_down(msr_read(IA32_MSR_APIC_BASE), PAGE_SIZE) |
        cpu->lapic_id << 12;
}
