/*
 * kernel/src/arch/aarch64/dev/time/time.c
 * © suhas pai
 */

#include "acpi/api.h"
#include "cpu/isr.h"
#include "dev/printk.h"

#include "lib/freq.h"

#include "sys/boot.h"
#include "sys/gic.h"

static uint64_t g_frequency = 0;

__optimize(3) static inline sec_t read_syscount() {
    uint64_t value = 0;
    asm volatile ("isb\n"
                  "mrs %0, cntpct_el0\n"
                  "isb"
                  : "=r"(value)
                  :: "memory");

    return value;
}

__unused __optimize(3) static inline sec_t read_virtcount() {
    uint64_t value = 0;
    asm volatile ("isb\n"
                  "mrs %0, cntvct_el0\n"
                  "isb"
                  : "=r"(value)
                  :: "memory");

    return value;
}

__optimize(3) nsec_t nsec_since_boot() {
    uint64_t cntvct = 0;
    asm volatile ("isb\n"
                  "mrs %0, cntvct_el0\n"
                  "isb"
                  : "=r"(cntvct)
                  :: "memory");

    return seconds_to_nano(read_syscount() / g_frequency -
                           (sec_t)boot_get_time());
}

__optimize(3) void oneshot_alarm(const nsec_t nano) {
    sec_t timestamp = 0;
    asm volatile ("mrs %0, cntpct_el0" : "=r"(timestamp));

    const uint64_t compare = timestamp + nano_to_seconds(g_frequency * nano);
    asm volatile ("msr cntp_cval_el0, %0" :: "r"(compare));
}

__optimize(3) static void
interrupt_handler(const uint64_t int_no, struct thread_context *const frame) {
    (void)int_no;
    (void)frame;

    printk(LOGLEVEL_INFO, "time: got interrupt\n");
}

static void enable_dtb_timer_irqs() {
    struct devicetree *const tree = dtb_get_tree();
    struct devicetree_node *const node =
        devicetree_get_node_at_path(tree, SV_STATIC("/timer"));

    if (node == NULL) {
        assert_msg(get_acpi_info()->gtdt != NULL,
                   "time: no timer found in acpi/dtb, expected GTDT table "
                   "in ACPI, or '/timer' node in dtb");
    }

    struct devicetree_prop_interrupts *const int_prop =
        (struct devicetree_prop_interrupts *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_INTERRUPTS);

    assert_msg(int_prop != NULL,
               "time: 'interrupts' prop not found in '/timer' dtb-node");

    uint8_t index = 0;
    array_foreach(&int_prop->list, const struct devicetree_prop_int_info, iter)
    {
        if (iter->polarity != IRQ_POLARITY_HIGH) {
            printk(LOGLEVEL_WARN,
                   "time: polarity for irq %" PRIu8 " isn't high polarity\n",
                   index);
            continue;
        }

        if (!range_has_loc(GIC_PPI_IRQ_RANGE, iter->num)) {
            printk(LOGLEVEL_WARN,
                   "time: irq %" PRIu8 " isn't a ppi interrupt\n",
                   index);
            continue;
        }

        isr_set_vector(iter->num, interrupt_handler, &(struct arch_isr_info){});

        gicd_set_irq_trigger_mode(iter->num, iter->trigger_mode);
        gicd_unmask_irq(iter->num);

        index++;
    }
}

void arch_init_time() {
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(g_frequency));
    printk(LOGLEVEL_INFO,
           "time: frequency is " FREQ_TO_UNIT_FMT "\n",
           FREQ_TO_UNIT_FMT_ARGS_ABBREV(g_frequency));

    // Enable and unmask generic timers
    asm volatile ("msr cntp_cval_el0, %0" :: "r"(UINT64_MAX));
    asm volatile ("msr cntp_ctl_el0, %0" :: "r"((uint64_t)1));
    asm volatile ("msr cntp_tval_el0, %0" :: "r"((uint64_t)0));

    asm volatile ("msr cntv_cval_el0, %0" :: "r"(UINT64_MAX));
    asm volatile ("msr cntv_ctl_el0, %0" :: "r"((uint64_t)1));

    enable_dtb_timer_irqs();

    printk(LOGLEVEL_INFO, "time: syscount is %" PRIu64 "\n", read_syscount());
    oneshot_alarm(20);
}