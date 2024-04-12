/*
 * kernel/src/arch/aarch64/dev/time/time.c
 * © suhas pai
 */

#include "dev/dtb/tree.h"
#include "sys/gic/api.h"

#include "acpi/api.h"
#include "cpu/isr.h"
#include "dev/printk.h"

#include "lib/freq.h"
#include "sched/scheduler.h"
#include "sys/boot.h"

static uint64_t g_frequency = 0;

enum cntp_ctl {
    __CNTP_CTL_ENABLE = 1 << 0,
    __CNTP_CTL_INT_MASK = 1 << 1,
    __CNTP_CTL_COND_MET = 1 << 2,
};

enum cntv_ctl {
    __CNTV_CTL_ENABLE = 1 << 0,
};

__optimize(3) nsec_t system_timer_get_count_ns() {
    nsec_t timestamp = 0;
    asm volatile ("mrs %0, cntpct_el0" : "=r"(timestamp));

    return timestamp;
}

__optimize(3) nsec_t nsec_since_boot() {
    uint64_t cntvct = 0;
    asm volatile ("isb\n"
                  "mrs %0, cntvct_el0\n"
                  "isb"
                  : "=r"(cntvct)
                  :: "memory");

    return seconds_to_nano(system_timer_get_count_ns() / g_frequency -
                           (sec_t)boot_get_time());
}

__optimize(3) uint64_t system_timer_get_frequency_ns() {
    return g_frequency;
}

__optimize(3) nsec_t system_timer_get_compare_ns() {
    nsec_t compare = 0;
    asm volatile ("mrs %0, cntp_cval_el0" : "=r"(compare));

    return compare;
}

__optimize(3) nsec_t system_timer_get_remaining_ns() {
    const nsec_t count = system_timer_get_count_ns();
    const nsec_t compare = system_timer_get_compare_ns();

    if (count > compare || count == UINT64_MAX) {
        return 0;
    }

    return (compare - count) / g_frequency;
}

__optimize(3) void system_timer_oneshot_ns(const nsec_t nano) {
    const usec_t tval = nano_to_seconds(g_frequency * nano);
    asm volatile ("msr cntp_tval_el0, %0" :: "r"(tval));
}

__optimize(3) void system_timer_stop_alarm() {
    asm volatile ("msr cntp_cval_el0, %0" :: "r"(UINT64_MAX));
}

__optimize(3) static void
interrupt_handler(const uint64_t intr_no, struct thread_context *const frame) {
    sched_next(frame, intr_no);
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

    struct devicetree_prop_interrupts *const intr_prop =
        (struct devicetree_prop_interrupts *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_INTERRUPTS);

    assert_msg(intr_prop != NULL,
               "time: 'interrupts' prop not found in '/timer' dtb-node");

    uint8_t index = 0;
    array_foreach(&intr_prop->list,
                  const struct devicetree_prop_intr_info,
                  iter)
    {
        if (iter->polarity != IRQ_POLARITY_HIGH) {
            printk(LOGLEVEL_WARN,
                   "time: polarity for irq %" PRIu8 " isn't high polarity\n",
                   index);
            continue;
        }

        if (!range_has_loc(GIC_PPI_IRQ_RANGE, iter->num)) {
            printk(LOGLEVEL_WARN,
                   "time: irq %" PRIu8 " is not a ppi interrupt\n",
                   index);
            continue;
        }

        isr_set_vector(iter->num, interrupt_handler, &ARCH_ISR_INFO_NONE());

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
    asm volatile ("msr cntp_ctl_el0, %0" :: "r"((uint64_t)__CNTP_CTL_ENABLE));

    asm volatile ("msr cntv_cval_el0, %0" :: "r"(UINT64_MAX));
    asm volatile ("msr cntv_ctl_el0, %0" :: "r"((uint64_t)__CNTV_CTL_ENABLE));

    if (boot_get_dtb() != NULL) {
        enable_dtb_timer_irqs();
    } else {
        assert_msg(get_acpi_info()->gtdt != NULL,
                   "time: dtb is missing and acpi is missing 'gtdt' table");
    }

    printk(LOGLEVEL_INFO,
           "time: syscount is %" PRIu64 "\n",
           system_timer_get_count_ns());
}