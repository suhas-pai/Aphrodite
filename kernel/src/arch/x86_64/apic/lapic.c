/*
 * kernel/src/arch/x86_64/apic/lapic.c
 * © suhas pai
 */

#include "acpi/api.h"

#include "asm/irqs.h"
#include "asm/msr.h"

#include "cpu/info.h"

#include "dev/pit.h"
#include "dev/printk.h"

#include "lib/freq.h"
#include "lib/time.h"

#include "sys/mmio.h"
#include "lapic.h"

static struct array g_lapic_list = ARRAY_INIT(sizeof(struct lapic_info));
volatile struct lapic_registers *lapic_regs = NULL;

__debug_optimize(3) static inline uint32_t
create_timer_register(const enum lapic_timer_mode timer_mode,
                      const uint8_t vector,
                      const bool masked)
{
    return (timer_mode << 17) | ((uint32_t)masked << 16) | vector;
}

static void calibrate_timer() {
    /*
     * Calibrate the Timer:
     *
     * We want to calibrate the timer to match the PIT, however the LAPIC timer
     * runs at a higher frequency than the PIT.
     *
     * Therefore, we setup both the LAPIC timer and the PIT to run for a sample
     * time, and after the lapic-timer reaches zero, we can calculate the
     * frequency based on the pit tick-count.
     *
     * To calculate the lapic-timer frequency, we first need to get the multiple
     * of the lapic-timer ticks per pit tick, in other words, how many
     * lapic-timer ticks for every one pit tick.
     *
     * From there, we can calculate the lapic-timer frequency by then using the
     * tick-multiple and multiplying it with the constant pit frequency to
     * obtain the lapic-timer frequency.
     */

    const bool flag = disable_irqs_if_enabled();

    const uint16_t pit_init_tick_number = pit_get_current_tick();
    const uint32_t sample_count = 0xFFFFFF;
    const uint32_t timer_reg =
        create_timer_register(LAPIC_TIMER_MODE_ONE_SHOT,
                              /*vector=*/0xFF,
                              /*masked=*/true);

    pit_set_reload_value(0xFFFF);
    if (get_acpi_info()->using_x2apic) {
        x2apic_write(X2APIC_LAPIC_REG_TIMER_CURR_COUNT, 0);
        x2apic_write(X2APIC_LAPIC_REG_TIMER_DIVIDE_CONFIG,
                     LAPIC_TIMER_DIV_CONFIG_BY_2);

        x2apic_write(X2APIC_LAPIC_REG_TIMER_INIT_COUNT, sample_count);
        x2apic_write(X2APIC_LAPIC_REG_LVT_TIMER, timer_reg);

        while (x2apic_read(X2APIC_LAPIC_REG_TIMER_CURR_COUNT) != 0) {}
    } else {
        mmio_write(&lapic_regs->timer_current_count, 0);
        mmio_write(&lapic_regs->timer_divide_config,
                   LAPIC_TIMER_DIV_CONFIG_BY_2);

        mmio_write(&lapic_regs->timer_initial_count, sample_count);
        mmio_write(&lapic_regs->lvt_timer, timer_reg);

        while (mmio_read(&lapic_regs->timer_current_count) != 0) {}
    }

    // Because the timer ticks down, init_tick_count > end_tick_count
    const uint16_t pit_end_tick_number = pit_get_current_tick();
    const uint16_t pit_tick_count = pit_init_tick_number - pit_end_tick_number;
    const uint32_t lapic_timer_freq_multiple = sample_count / pit_tick_count;

    this_cpu_mut()->lapic_timer_frequency =
        lapic_timer_freq_multiple * PIT_FREQUENCY;

    enable_irqs_if_flag(flag);
}

__debug_optimize(3) uint32_t x2apic_read(const enum x2apic_reg reg) {
    return msr_read(IA32_MSR_X2APIC_BASE + reg);
}

__debug_optimize(3)
void x2apic_write(const enum x2apic_reg reg, const uint64_t value) {
    msr_write(IA32_MSR_X2APIC_BASE + reg, value);
}

__debug_optimize(3) void adjust_lint_extint_value(uint64_t *const value_in) {
    const uint64_t add_mask = APIC_LVT_DELIVERY_MODE_EXTINT << 8;
    const uint64_t remove_mask = 0b111 << 8 | 1 << 12 | 1 << 14 | 1 << 16;

    *value_in = rm_mask(*value_in, remove_mask) | add_mask;
}

__debug_optimize(3) void adjust_lint_nmi_value(uint64_t *const value_in) {
    const uint64_t add_mask = APIC_LVT_DELIVERY_MODE_NMI << 8;
    *value_in = rm_mask(*value_in, 0b111 << 8 | 1 << 15) | add_mask;
}

void lapic_enable() {
    const uint32_t spur_vector_mask =
        (uint32_t)isr_get_spur_vector() | __LAPIC_SPURVEC_ENABLE;

    if (get_acpi_info()->using_x2apic) {
        uint64_t lint0_value = x2apic_read(X2APIC_LAPIC_REG_LVT_LINT0);
        uint64_t lint1_value = x2apic_read(X2APIC_LAPIC_REG_LVT_LINT1);

        if (get_acpi_info()->nmi_lint == 1) {
            adjust_lint_extint_value(&lint0_value);
            adjust_lint_nmi_value(&lint1_value);
        } else {
            adjust_lint_extint_value(&lint1_value);
            adjust_lint_nmi_value(&lint0_value);
        }

        const uint64_t spur_vector_value =
            x2apic_read(X2APIC_LAPIC_REG_SPUR_VECTOR);

        x2apic_write(X2APIC_LAPIC_REG_SPUR_VECTOR,
                     spur_vector_value | spur_vector_mask);

        x2apic_write(X2APIC_LAPIC_REG_LVT_LINT0, lint0_value);
        x2apic_write(X2APIC_LAPIC_REG_LVT_LINT1, lint1_value);
    } else {
        uint64_t lint0_value = mmio_read(&lapic_regs->lvt_lint0);
        uint64_t lint1_value = mmio_read(&lapic_regs->lvt_lint1);

        if (get_acpi_info()->nmi_lint == 1) {
            adjust_lint_extint_value(&lint0_value);
            adjust_lint_nmi_value(&lint1_value);
        } else {
            adjust_lint_extint_value(&lint1_value);
            adjust_lint_nmi_value(&lint0_value);
        }

        mmio_write(&lapic_regs->lvt_lint0, (uint32_t)lint0_value);
        mmio_write(&lapic_regs->lvt_lint1, (uint32_t)lint1_value);
        mmio_write(&lapic_regs->spur_intr_vector, spur_vector_mask);
    }

    printk(LOGLEVEL_INFO, "apic: lapic enabled\n");
}

__debug_optimize(3) void lapic_eoi() {
    if (get_acpi_info()->using_x2apic) {
        x2apic_write(X2APIC_LAPIC_REG_EOI, 0);
    } else if (__builtin_expect(lapic_regs != NULL, 1)) {
        mmio_write(&lapic_regs->eoi, /*value=*/0);
    }
}

__debug_optimize(3)
void lapic_send_ipi(const uint32_t lapic_id, const uint32_t vector) {
    if (get_acpi_info()->using_x2apic) {
        x2apic_write(X2APIC_LAPIC_REG_ICR, (uint64_t)lapic_id << 32 | vector);
    } else {
        mmio_write(&lapic_regs->icr[1].value, lapic_id << 24);
        mmio_write(&lapic_regs->icr[0].value, vector);
    }
}

__debug_optimize(3) void lapic_send_self_ipi(const uint32_t vector) {
    if (get_acpi_info()->using_x2apic) {
        x2apic_write(X2APIC_LAPIC_REG_SELF_IPI, vector);
    } else {
        preempt_disable();
        const uint32_t lapic_id = this_cpu()->lapic_id;
        preempt_enable();

        lapic_send_ipi(lapic_id, vector);
    }
}

__debug_optimize(3) void lapic_timer_stop() {
    if (get_acpi_info()->using_x2apic) {
        x2apic_write(X2APIC_LAPIC_REG_TIMER_INIT_COUNT, 0);
        x2apic_write(X2APIC_LAPIC_REG_LVT_TIMER,
                     create_timer_register(LAPIC_TIMER_MODE_ONE_SHOT,
                                           /*vector=*/isr_get_timer_vector(),
                                           /*masked=*/true));
    } else {
        mmio_write(&lapic_regs->timer_initial_count, 0);
        mmio_write(&lapic_regs->lvt_timer,
                   create_timer_register(LAPIC_TIMER_MODE_ONE_SHOT,
                                         /*vector=*/isr_get_timer_vector(),
                                         /*masked=*/true));
    }
}

__debug_optimize(3) usec_t lapic_timer_remaining() {
    preempt_disable();
    const uint64_t lapic_timer_freq_in_microseconds =
        this_cpu()->lapic_timer_frequency / MICRO_IN_SECONDS;

    preempt_enable();
    if (get_acpi_info()->using_x2apic) {
        return x2apic_read(X2APIC_LAPIC_REG_TIMER_INIT_COUNT)
               / lapic_timer_freq_in_microseconds;
    }

    return mmio_read(&lapic_regs->timer_initial_count)
           / lapic_timer_freq_in_microseconds;
}

__debug_optimize(3)
void lapic_timer_one_shot(const usec_t usec, const isr_vector_t vector) {
    // LAPIC-Timer Frequency is in Hz, which is cycles per second, while we need
    // cycles per microseconds

    const uint64_t lapic_timer_freq_in_us =
        this_cpu()->lapic_timer_frequency / MICRO_IN_SECONDS;

    const uint64_t count = check_mul_assert(lapic_timer_freq_in_us, usec);
    if (get_acpi_info()->using_x2apic) {
        x2apic_write(X2APIC_LAPIC_REG_TIMER_INIT_COUNT, count);
        x2apic_write(X2APIC_LAPIC_REG_LVT_TIMER,
                     create_timer_register(LAPIC_TIMER_MODE_ONE_SHOT,
                                           vector,
                                           /*masked=*/false));
    } else {
        mmio_write(&lapic_regs->timer_initial_count, count);
        mmio_write(&lapic_regs->lvt_timer,
                   create_timer_register(LAPIC_TIMER_MODE_ONE_SHOT,
                                         vector,
                                         /*masked=*/false));
    }
}

void lapic_init() {
    const bool flag = disable_irqs_if_enabled();

    lapic_enable();
    lapic_timer_stop();

    calibrate_timer();
    lapic_timer_stop();

    printk(LOGLEVEL_INFO,
           "lapic: frequency is " FREQ_TO_UNIT_FMT "\n",
           FREQ_TO_UNIT_FMT_ARGS_ABBREV(this_cpu()->lapic_timer_frequency));

    enable_irqs_if_flag(flag);
}

void lapic_add(const struct lapic_info *const lapic_info) {
    const uint64_t index = array_item_count(g_lapic_list);
    if (!lapic_info->enabled) {
        if (lapic_info->online_capable) {
            printk(LOGLEVEL_INFO,
                   "lapic: cpu #%" PRIu64 " (with processor id: %" PRIu8 " and "
                   "local-apic id: %" PRIu8 ") has flag bit 0 disabled, but "
                   "CAN be enabled\n",
                   index,
                   lapic_info->processor_id,
                   lapic_info->apic_id);
        } else {
            printk(LOGLEVEL_INFO,
                   "lapic: \tcpu #%" PRIu64 " (with processor id: %" PRIu8
                   "and local-apic id: %" PRIu8 ") CANNOT be enabled\n",
                   index,
                   lapic_info->processor_id,
                   lapic_info->apic_id);
        }
    } else {
        printk(LOGLEVEL_INFO,
               "lapic: cpu #%" PRIu64 " (with processor id: %" PRIu8 " and "
               "local-apic id: %" PRIu8 ") CAN be enabled\n",
               index,
               lapic_info->processor_id,
               lapic_info->apic_id);
    }

    assert_msg(array_append(&g_lapic_list, lapic_info),
               "lapic: failed to add local-apic info to array");
}