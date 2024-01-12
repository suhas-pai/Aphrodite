/*
 * kernel/src/arch/aarch64/dev/time/time.c
 * © suhas pai
 */

#include "acpi/api.h"
#include "asm/irqs.h"
#include "dev/printk.h"

#include "lib/freq.h"
#include "lib/time.h"

#include "sys/boot.h"
#include "sys/gic.h"

enum ctl_flags {
    __CTL_ENABLE = 1ull << 0,
    __CTL_INT_MASKED = 1ull << 1,
    __CTL_STATUS = 1ull << 2,
};

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

void oneshot_alarm(const nsec_t nano) {
    const sec_t seconds = nano_to_seconds(g_frequency * nano);
    asm volatile ("msr cntp_tval_el0, %0" :: "r"(seconds));

    printk(LOGLEVEL_INFO, "time: seconds is %" PRIu64 "\n", seconds);

    uint64_t tval = 1;
    asm volatile ("mrs %0, cntp_tval_el0" : "+r"(tval));

    printk(LOGLEVEL_INFO, "time: tval is %" PRIu64 "\n", tval);
}

static void enable_dtb_timer_irqs() {
    struct devicetree *const tree = dtb_get_tree();
    struct devicetree_node *const node =
        devicetree_get_node_at_path(tree, SV_STATIC("/timer"));

    if (node == NULL) {
        if (get_acpi_info()->gtdt == NULL) {
            panic("time: no timer found in acpi/dtb, expected GTDT table "
                  "in ACPI, or '/timer' node in dtb");
        }

        return;
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

    printk(LOGLEVEL_INFO, "time: syscount is %" PRIu64 "\n", read_syscount());

    enable_interrupts();
    enable_dtb_timer_irqs();

    oneshot_alarm(0);
}