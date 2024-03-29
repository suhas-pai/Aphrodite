/*
 * kernel/src/arch/x86_64/sys/isr.c
 * © suhas pai
 */

#include "lib/adt/bitset.h"

#include "apic/ioapic.h"
#include "apic/lapic.h"

#include "asm/msr.h"
#include "cpu/isr.h"

#include "dev/printk.h"

#include "lib/align.h"
#include "lib/util.h"

#define ISR_EXCEPTION_COUNT 32
#define ISR_IRQ_COUNT 256

static struct spinlock g_lock = SPINLOCK_INIT();

static bitset_decl(g_bitset, ISR_IRQ_COUNT);
static isr_func_t g_funcs[ISR_IRQ_COUNT] = {0};

static isr_vector_t g_spur_vector = 0;
static isr_vector_t g_timer_vector = 0;

__optimize(3) isr_vector_t isr_alloc_vector(const bool for_msi) {
    (void)for_msi;

    const int flag = spin_acquire_irq_save(&g_lock);
    const uint64_t result =
        bitset_find_unset(g_bitset, ISR_IRQ_COUNT, /*invert=*/true);

    spin_release_irq_restore(&g_lock, flag);
    if (result == BITSET_INVALID) {
        return ISR_INVALID_VECTOR;
    }

    printk(LOGLEVEL_INFO,
           "isr: allocated vector " ISR_VECTOR_FMT "\n",
           (isr_vector_t)result);

    return (isr_vector_t)result;
}

__optimize(3)
void isr_free_vector(const isr_vector_t vector, const bool for_msi) {
    (void)for_msi;
    assert_msg(vector > ISR_EXCEPTION_COUNT,
               "isr_free_vector() called on x86 exception vector");

    const int flag = spin_acquire_irq_save(&g_lock);

    bitset_unset(g_bitset, vector);
    isr_set_vector(vector, /*handler=*/NULL, &ARCH_ISR_INFO_NONE());

    spin_release_irq_restore(&g_lock, flag);
    printk(LOGLEVEL_INFO, "isr: freed vector " ISR_VECTOR_FMT "\n", vector);
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

extern void
handle_exception(const uint64_t vector, struct thread_context *const frame);

__optimize(3) void
isr_handle_interrupt(const uint64_t vector,
                     struct thread_context *const frame)
{
    if (g_funcs[vector] != NULL) {
        g_funcs[vector](vector, frame);
    } else {
        if (index_in_bounds(vector, ISR_EXCEPTION_COUNT)) {
            handle_exception(vector, frame);
        } else {
            printk(LOGLEVEL_INFO,
                   "Got unhandled interrupt %" PRIu64 "\n",
                   vector);
        }

        lapic_eoi();
    }
}

__optimize(3) static
void spur_tick(const uint64_t intr_no, struct thread_context *const frame) {
    (void)intr_no;
    (void)frame;

    this_cpu_mut()->spur_intr_count++;
    lapic_eoi();
}

void isr_init() {
    // Set first 32 exception interrupts as allocated.
    g_bitset[0] = mask_for_n_bits(ISR_EXCEPTION_COUNT);

    // Setup Timer
    g_timer_vector = isr_alloc_vector(/*for_msi=*/false);
    assert(g_timer_vector != ISR_INVALID_VECTOR);

    // Setup Spurious Interrupt
    g_spur_vector = isr_alloc_vector(/*for_msi=*/false);
    assert(g_spur_vector != ISR_INVALID_VECTOR);

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
}

__optimize(3) void
isr_assign_irq_to_cpu(const struct cpu_info *const cpu,
                      const uint8_t irq,
                      const isr_vector_t vector,
                      const bool masked)
{
    ioapic_redirect_irq(cpu->lapic_id, irq, vector, masked);
}

__optimize(3) void isr_eoi(const uint64_t intr_no) {
    (void)intr_no;
    lapic_eoi();
}

__optimize(3) uint64_t
isr_get_msi_address(const struct cpu_info *const cpu, const isr_vector_t vector)
{
    (void)vector;
    return
        align_down(msr_read(IA32_MSR_APIC_BASE), PAGE_SIZE)
        | cpu->lapic_id << 12;
}

__optimize(3) uint64_t
isr_get_msix_address(const struct cpu_info *const cpu,
                     const isr_vector_t vector)
{
    (void)vector;
    return
        align_down(msr_read(IA32_MSR_APIC_BASE), PAGE_SIZE)
        | cpu->lapic_id << 12;
}
