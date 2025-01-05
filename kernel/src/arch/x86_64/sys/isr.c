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

#include "sched/thread.h"
#include "sys/irq.h"

#define ISR_EXCEPTION_COUNT 32
#define ISR_INT_COUNT 256

#define ISR_IRQ_COUNT (ISR_INT_COUNT - ISR_EXCEPTION_COUNT)

struct isr_func_info {
    isr_func_t handler;
    void *ctx;

    bool masked : 1;
};

static struct spinlock g_lock = SPINLOCK_INIT();
static bitset_decl(g_vector_bitset, ISR_INT_COUNT);

static struct isr_func_info g_funcs[ISR_INT_COUNT] = {0};
static struct irq_pin g_irq_pin_list[ISR_IRQ_COUNT] = {0};

static isr_vector_t g_spur_vector = 0;
static isr_vector_t g_lapic_vector = 0;
static isr_vector_t g_hpet_vector = 0;

void isr_setup_irq_pins() {
    array_foreach(&get_acpi_info()->iso_list, const struct apic_iso_info, iso) {
        const enum irq_polarity polarity =
            iso->flags & __ACPI_MADT_ENTRY_ISO_ACTIVE_LOW ?
                IRQ_POLARITY_LOW : IRQ_POLARITY_HIGH;

        const enum irq_trigger_mode trigger_mode =
            iso->flags & __ACPI_MADT_ENTRY_ISO_LEVEL_TRIGGER ?
                IRQ_TRIGGER_MODE_LEVEL : IRQ_TRIGGER_MODE_EDGE;

        printk(LOGLEVEL_INFO,
               "isr: setting up irq pin for apic iso:\n"
               "\tirq %d -> %d\n"
               "\tbus: %d\n"
               "\tpolarity: %s\n"
               "\ttrigger mode: %s\n",
               iso->irq_src,
               iso->gsi,
               iso->bus_src,
               polarity == IRQ_POLARITY_LOW ? "low" : "high",
               trigger_mode == IRQ_TRIGGER_MODE_LEVEL ? "level" : "edge");

        g_irq_pin_list[iso->irq_src].polarity = polarity;
        g_irq_pin_list[iso->irq_src].trigger_mode = trigger_mode;
    }
}

__debug_optimize(3) isr_vector_t isr_alloc_vector() {
    uint64_t bit_index = 0;
    with_spinlock_intr_disabled(&g_lock, {
        bit_index =
            bitset_find_unset(g_vector_bitset, ISR_INT_COUNT, /*invert=*/true);
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

    with_spinlock_intr_disabled(&g_lock, {
        bitset_unset(g_vector_bitset, vector);
        isr_set_vector(vector,
                       /*handler=*/nullptr,
                       /*ctx=*/nullptr,
                       &ARCH_ISR_INFO_NONE());
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

__debug_optimize(3) void isr_mask_irq(struct irq_pin *const pin) {
    ioapic_toggle_irq_mask(pin->irq, /*masked=*/true);
}

__debug_optimize(3) void isr_unmask_irq(struct irq_pin *const pin) {
    ioapic_toggle_irq_mask(pin->irq, /*masked=*/false);
}

__debug_optimize(3) void isr_mask_intr(const isr_vector_t vector) {
    g_funcs[vector].masked = true;
}

__debug_optimize(3) void isr_unmask_intr(const isr_vector_t vector) {
    g_funcs[vector].masked = false;
}

extern void
handle_exception(const uint64_t vector, struct thread_context *const frame);

__debug_optimize(3) void
isr_handle_interrupt(const uint64_t vector, struct thread_context *const frame)
{
    struct isr_func_info *const info = &g_funcs[vector];
    this_cpu_mut()->called_eoi = false;

    if (info->masked) {
        lapic_eoi();
        return;
    }

    if (__builtin_expect(info->handler != nullptr, 1)) {
        info->handler(vector, frame, info->ctx);
        if (!this_cpu()->called_eoi) {
            printk(LOGLEVEL_WARN,
                   "isr: handler for vector " ISR_VECTOR_FMT " didn't call "
                   "eoi\n",
                   (isr_vector_t)vector);

            lapic_eoi();
        }

        return;
    }

    if (index_in_bounds(vector, ISR_EXCEPTION_COUNT)) {
        handle_exception(vector, frame);
        return;
    }

    printk(LOGLEVEL_WARN,
           "isr: got unhandled interrupt " ISR_VECTOR_FMT "\n",
           (isr_vector_t)vector);

    lapic_eoi();
}

__debug_optimize(3) static void
spur_tick(const uint64_t intr_no,
          struct thread_context *const frame,
          void *const ctx)
{
    (void)intr_no;
    (void)frame;
    (void)ctx;

    this_cpu_mut()->spur_intr_count++;
    lapic_eoi();
}

void isr_init() {
    // Set first 32 exception interrupts as allocated.
    g_vector_bitset[0] = mask_for_n_bits(ISR_EXCEPTION_COUNT);

    // Setup LAPIC Interrupt
    g_lapic_vector = isr_alloc_vector();
    assert(g_lapic_vector != ISR_INVALID_VECTOR);

    // Setup Spurious Interrupt
    g_spur_vector = isr_alloc_vector();
    assert(g_spur_vector != ISR_INVALID_VECTOR);

    // Setup HPET Interrupt
    g_hpet_vector = isr_alloc_vector();
    assert(g_hpet_vector != ISR_INVALID_VECTOR);

    isr_set_vector(g_spur_vector,
                   spur_tick,
                   /*ctx=*/nullptr,
                   &ARCH_ISR_INFO_NONE());

    idt_register_exception_handlers();
}

__debug_optimize(3) void
isr_set_vector(const isr_vector_t vector,
               const isr_func_t handler,
               void *const ctx,
               struct arch_isr_info *const info)
{
    g_funcs[vector].handler = handler;
    g_funcs[vector].ctx = ctx;

    idt_set_vector(vector, info->ist, IDT_DEFAULT_FLAGS);
}

__debug_optimize(3) void
isr_set_msi_vector(const isr_vector_t vector,
                   const isr_func_t handler,
                   void *const ctx,
                   struct arch_isr_info *const info)
{
    isr_set_vector(vector, handler, ctx, info);
}

struct irq_pin *isr_get_irq_pin(const uint16_t irq) {
    assert(index_in_bounds(irq, countof(g_irq_pin_list)));
    return &g_irq_pin_list[irq];
}

bool
isr_install_irq(struct irq_pin *const pin,
                const isr_func_t handler,
                void *const ctx,
                const bool masked)
{
    const isr_vector_t vector = isr_alloc_vector();
    if (vector == ISR_INVALID_VECTOR) {
        return false;
    }

    with_preempt_disabled({
        const struct cpu_info *const cpu = this_cpu();
        ioapic_redirect_irq(cpu->lapic_id, pin->irq, vector, masked);
    });

    isr_set_vector(vector, handler, ctx, &ARCH_ISR_INFO_NONE());
    return true;
}

void *isr_uninstall_irq(struct irq_pin *const pin) {
    void *const result = g_funcs[pin->irq].ctx;

    isr_free_vector(pin->vector);
    pin->vector = ISR_INVALID_VECTOR;

    return result;
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
    const struct os_acpi_fadt *const fadt = get_acpi_info()->fadt;
    if (fadt != nullptr) {
        if (fadt->iapc_boot_arch_flags &
                __ACPI_FADT_IAPC_BOOT_MSI_NOT_SUPPORTED)
        {
            return ISR_MSI_SUPPORT_NONE;
        }
    }

    return ISR_MSI_SUPPORT_MSIX;
}
