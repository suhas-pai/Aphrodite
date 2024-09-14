/*
 * kernel/src/arch/x86_64/sys/isr.c
 * © suhas pai
 */

#include "lib/adt/bitset.h"
#include "acpi/api.h"

#include "apic/ioapic.h"
#include "apic/lapic.h"

#include "asm/msr.h"

#include "cpu/isr.h"
#include "cpu/spinlock.h"

#include "dev/printk.h"

#include "lib/align.h"
#include "lib/util.h"

#define ISR_EXCEPTION_COUNT 32
#define ISR_IRQ_COUNT 256

static struct spinlock g_lock = SPINLOCK_INIT();

static bitset_decl(g_bitset, ISR_IRQ_COUNT);
static isr_func_t g_funcs[ISR_IRQ_COUNT] = {0};

static isr_vector_t g_spur_vector = 0;
static isr_vector_t g_lapic_vector = 0;
static isr_vector_t g_hpet_vector = 0;

__debug_optimize(3) isr_vector_t isr_alloc_vector() {
    uint64_t bit_index = 0;
    with_spinlock_irq_disabled(&g_lock, {
        bit_index = bitset_find_unset(g_bitset, ISR_IRQ_COUNT, /*invert=*/true);
    });

    if (bit_index == BITSET_INVALID) {
        return ISR_INVALID_VECTOR;
    }

    const isr_vector_t vector = (isr_vector_t)bit_index;
    printk(LOGLEVEL_INFO, "isr: allocated vector " ISR_VECTOR_FMT "\n", vector);

    return vector;
}

__debug_optimize(3) isr_vector_t
isr_alloc_msi_vector(struct device *const device, const uint16_t msi_index) {
    (void)device;
    (void)msi_index;

    return isr_alloc_vector();
}

__debug_optimize(3) void isr_free_vector(const isr_vector_t vector) {
    assert_msg(vector > ISR_EXCEPTION_COUNT,
               "isr_free_vector() called on x86 exception vector");

    with_spinlock_irq_disabled(&g_lock, {
        bitset_unset(g_bitset, vector);
        isr_set_vector(vector, /*handler=*/NULL, &ARCH_ISR_INFO_NONE());
    });

    printk(LOGLEVEL_INFO, "isr: freed vector " ISR_VECTOR_FMT "\n", vector);
}

__debug_optimize(3) void
isr_free_msi_vector(struct device *const device,
                    const isr_vector_t vector,
                    const uint16_t msi_index)
{
    (void)device;
    (void)msi_index;

    return isr_free_vector(vector);
}

__debug_optimize(3) isr_vector_t isr_get_lapic_vector() {
    return g_lapic_vector;
}

__debug_optimize(3) isr_vector_t isr_get_hpet_vector() {
    return g_hpet_vector;
}

__debug_optimize(3) isr_vector_t isr_get_spur_vector() {
    return g_spur_vector;
}

__debug_optimize(3) void isr_mask_irq(const isr_vector_t irq) {
    (void)irq;
}

__debug_optimize(3) void isr_unmask_irq(const isr_vector_t irq) {
    (void)irq;
}

extern void
handle_exception(const uint64_t vector, struct thread_context *const frame);

__debug_optimize(3) void
isr_handle_interrupt(const uint64_t vector, struct thread_context *const frame)
{
    if (__builtin_expect(g_funcs[vector] != NULL, 1)) {
        g_funcs[vector](vector, frame);
        return;
    }

    if (index_in_bounds(vector, ISR_EXCEPTION_COUNT)) {
        handle_exception(vector, frame);
        return;
    }

    printk(LOGLEVEL_INFO, "isr: got unhandled interrupt %" PRIu64 "\n", vector);
    lapic_eoi();
}

__debug_optimize(3) static
void spur_tick(const uint64_t intr_no, struct thread_context *const frame) {
    (void)intr_no;
    (void)frame;

    this_cpu_mut()->spur_intr_count++;
    lapic_eoi();
}

void isr_init() {
    // Set first 32 exception interrupts as allocated.
    g_bitset[0] = mask_for_n_bits(ISR_EXCEPTION_COUNT);

    // Setup LAPIC Interrupt
    g_lapic_vector = isr_alloc_vector();
    assert(g_lapic_vector != ISR_INVALID_VECTOR);

    // Setup Spurious Interrupt
    g_spur_vector = isr_alloc_vector();
    assert(g_spur_vector != ISR_INVALID_VECTOR);

    // Setup HPET Interrupt
    g_hpet_vector = isr_alloc_vector();
    assert(g_hpet_vector != ISR_INVALID_VECTOR);

    isr_set_vector(g_spur_vector, spur_tick, &ARCH_ISR_INFO_NONE());
    idt_register_exception_handlers();
}

__debug_optimize(3) void
isr_set_vector(const isr_vector_t vector,
               const isr_func_t handler,
               struct arch_isr_info *const info)
{
    g_funcs[vector] = handler;
    idt_set_vector(vector, info->ist, IDT_DEFAULT_FLAGS);
}

__debug_optimize(3) void
isr_set_msi_vector(const isr_vector_t vector,
                   const isr_func_t handler,
                   struct arch_isr_info *const info)
{
    isr_set_vector(vector, handler, info);
}

__debug_optimize(3) void
isr_assign_irq_to_cpu(const struct cpu_info *const cpu,
                      const uint8_t irq,
                      const isr_vector_t vector,
                      const bool masked)
{
    ioapic_redirect_irq(cpu->lapic_id, irq, vector, masked);
}

__debug_optimize(3) void isr_eoi(const uint64_t intr_no) {
    (void)intr_no;
    lapic_eoi();
}

__debug_optimize(3) uint64_t
isr_get_msi_address(const struct cpu_info *const cpu, const isr_vector_t vector)
{
    (void)vector;
    return
        align_down(msr_read(IA32_MSR_APIC_BASE), PAGE_SIZE)
      | cpu->lapic_id << 12;
}

__debug_optimize(3) uint64_t
isr_get_msix_address(const struct cpu_info *const cpu,
                     const isr_vector_t vector)
{
    (void)vector;
    return
        align_down(msr_read(IA32_MSR_APIC_BASE), PAGE_SIZE)
      | cpu->lapic_id << 12;
}

__debug_optimize(3) enum isr_msi_support isr_get_msi_support() {
    const struct acpi_fadt *const fadt = get_acpi_info()->fadt;
    if (fadt != NULL) {
        if (fadt->iapc_boot_arch_flags
              & __ACPI_FADT_IAPC_BOOT_MSI_NOT_SUPPORTED)
        {
            return ISR_MSI_SUPPORT_NONE;
        }
    }

    return ISR_MSI_SUPPORT_MSIX;
}
