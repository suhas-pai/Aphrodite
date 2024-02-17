/*
 * kernel/src/arch/aarch64/sys/isr.c
 * © suhas pai
 */

#include "lib/adt/bitmap.h"

#include "cpu/isr.h"
#include "dev/printk.h"

#include "sys/gic.h"

#define ISR_IRQ_COUNT 1020

struct irq_info {
    isr_func_t handler;

    bool alloced_in_msi : 1;
    bool for_msi : 1;
};

extern void *const ivt_el1;

static struct bitmap g_bitmap = BITMAP_INIT();;
static struct irq_info g_irq_info_list[ISR_IRQ_COUNT] = {0};

void isr_init() {
    g_bitmap = bitmap_alloc(ISR_IRQ_COUNT);
}

__optimize(3)
void isr_reserve_msi_irqs(const uint16_t base, const uint16_t count) {
    uint16_t end = 0;
    assert(check_add(base, count, &end) && end <= ISR_IRQ_COUNT);

    for (uint16_t irq = base; irq != base + count; irq++) {
        g_irq_info_list[irq].for_msi = true;

        gicd_set_irq_priority(irq, IRQ_POLARITY_HIGH);
        gicd_set_irq_trigger_mode(irq, IRQ_TRIGGER_MODE_EDGE);
    }

    bitmap_set_range(&g_bitmap, RANGE_INIT(base, count), /*value=*/true);
}

void isr_install_vbar() {
    asm volatile ("msr vbar_el1, %0" :: "r"(&ivt_el1));
    printk(LOGLEVEL_INFO, "isr: installed vbar_el1\n");
}

__optimize(3) isr_vector_t isr_alloc_vector(const bool for_msi) {
    if (for_msi) {
        struct gic_v2_msi_info *iter = NULL;
        struct list *const list = gicd_get_msi_info_list();

        list_foreach(iter, list, list) {
            const uint16_t end = iter->spi_base + iter->spi_count;
            for (uint16_t irq = iter->spi_base; irq != end; irq++) {
                assert(g_irq_info_list[irq].for_msi);
                if (!g_irq_info_list[irq].alloced_in_msi) {
                    g_irq_info_list[irq].alloced_in_msi = true;
                    return irq;
                }
            }
        }

        verify_not_reached();
    }

    (void)for_msi;
    const uint64_t result =
        bitmap_find(&g_bitmap,
                    /*count=*/1,
                    /*start_index=*/GIC_SPI_INTERRUPT_START,
                    /*expected_value=*/false,
                    /*value=*/true);

    assert_msg(result != FIND_BIT_INVALID, "isr: ran out of free vectors");
    return (isr_vector_t)result;
}

__optimize(3) void
isr_set_vector(const isr_vector_t vector,
               const isr_func_t handler,
               struct arch_isr_info *const info)
{
    (void)info;

    g_irq_info_list[vector].handler = handler;
    printk(LOGLEVEL_INFO,
           "isr: registered handler for vector %" PRIu8 "\n",
           vector);
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

    verify_not_reached();
}

__optimize(3) void isr_mask_irq(const isr_vector_t irq) {
    gicd_mask_irq(irq);
}

__optimize(3) void isr_unmask_irq(const isr_vector_t irq) {
    gicd_unmask_irq(irq);
}

void handle_exception(irq_context_t *const context) {
    (void)context;

    uint8_t cpu_id = 0;
    const int irq = gic_cpu_get_irq_number(&cpu_id);

    printk(LOGLEVEL_INFO, "isr: got exception: %d on cpu: %d\n", irq, cpu_id);
    gic_cpu_eoi(cpu_id, irq);
}

void handle_sexception(irq_context_t *const context) {
    (void)context;

    uint8_t cpu_id = 0;
    const int irq = gic_cpu_get_irq_number(&cpu_id);

    printk(LOGLEVEL_INFO, "isr: got sexception: %d on cpu: %d\n", irq, cpu_id);
    gic_cpu_eoi(cpu_id, irq);
}

__optimize(3) void handle_interrupt(irq_context_t *const context) {
    uint8_t cpu_id = 0;
    const irq_number_t irq = gic_cpu_get_irq_number(&cpu_id);

    if (irq >= ISR_IRQ_COUNT) {
        printk(LOGLEVEL_WARN,
               "isr: got spurious interrupt " ISR_VECTOR_FMT " on "
               "cpu %" PRIu8 "\n",
               irq,
               cpu_id);

        cpu_mut_for_intr_number(cpu_id)->spur_int_count++;
        gic_cpu_eoi(cpu_id, irq);

        return;
    }

    const isr_func_t handler = g_irq_info_list[irq].handler;
    if (handler != NULL) {
        handler((uint64_t)cpu_id << 16 | irq, context);
    } else {
        printk(LOGLEVEL_WARN,
               "isr: got unhandled interrupt " ISR_VECTOR_FMT " on "
               "cpu %" PRIu8 "\n",
               irq,
               cpu_id);
    }
}

__optimize(3) void isr_eoi(const uint64_t int_info) {
    gic_cpu_eoi(int_info >> 16, /*irq_number=*/(uint16_t)int_info);
}

__optimize(3) enum isr_msi_support isr_get_msi_support() {
    return ISR_MSI_SUPPORT_MSIX;
}

__optimize(3) uint64_t
isr_get_msi_address(const struct cpu_info *const cpu,
                    const isr_vector_t vector)
{
    (void)cpu;
    return (uint64_t)gicd_get_msi_address(vector);
}

__optimize(3) uint64_t
isr_get_msix_address(const struct cpu_info *const cpu,
                    const isr_vector_t vector)
{
    (void)cpu;
    return (uint64_t)gicd_get_msi_address(vector);
}
