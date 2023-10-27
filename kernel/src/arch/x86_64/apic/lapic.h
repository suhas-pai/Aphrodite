/*
 * kernel/arch/x86_64/apic/lapic.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include "apic/structs.h"

enum lapic_version_reg_flags {
    __LAPIC_VERSION_REG_VERION_MASK = 0xFF,
    __LAPIC_VERSION_REG_MAX_LVT_ENTRIES_MASK = 0xFF0000,
    __LAPIC_VERSION_REG_SUPPRESS_EOI = (1 << 24),
};

enum lapic_reg {
    // ICR = "Interrupt Command Register"
    LAPIC_REG_ID = 0x20,
    LAPIC_REG_VERSION = 0x30,

    // TPR = "Task Priority Register"
    LAPIC_REG_TPR = 0x80,

    // APR = "Arbitration Priority Register"
    LAPIC_REG_APR = 0x90,

    // PPR = "Processor Priority Register"
    LAPIC_REG_PPR = 0xA0,
    LAPIC_REG_EOI = 0xB0,

    // RRD = "Remote Read Register"
    LAPIC_REG_RRD = 0xC0,

    // LDR = "Logical Destination Register"
    LAPIC_REG_LDR = 0xD0,

    LAPIC_REG_SPURIOUS = 0xF0,

    // ISR = "In-Service Register"
    LAPIC_REG_ISR_0 = 0x100,
    LAPIC_REG_ISR_1 = 0x110,
    LAPIC_REG_ISR_2 = 0x120,
    LAPIC_REG_ISR_3 = 0x130,
    LAPIC_REG_ISR_4 = 0x140,
    LAPIC_REG_ISR_5 = 0x150,
    LAPIC_REG_ISR_6 = 0x160,
    LAPIC_REG_ISR_7 = 0x170,

    // TMR = "Trigger Mode Register"
    LAPIC_REG_TMR_0 = 0x180,
    LAPIC_REG_TMR_1 = 0x190,
    LAPIC_REG_TMR_2 = 0x1A0,
    LAPIC_REG_TMR_3 = 0x1B0,
    LAPIC_REG_TMR_4 = 0x1C0,
    LAPIC_REG_TMR_5 = 0x1D0,
    LAPIC_REG_TMR_6 = 0x1E0,
    LAPIC_REG_TMR_7 = 0x1F0,

    // IRR = "Interrupt Request Register"
    LAPIC_REG_IRR_0 = 0x200,
    LAPIC_REG_IRR_1 = 0x210,
    LAPIC_REG_IRR_2 = 0x220,
    LAPIC_REG_IRR_3 = 0x230,
    LAPIC_REG_IRR_4 = 0x240,
    LAPIC_REG_IRR_5 = 0x250,
    LAPIC_REG_IRR_6 = 0x260,
    LAPIC_REG_IRR_7 = 0x270,

    LAPIC_REG_ERROR_STATUS = 0x280,

    // LVT = "Local Vector Table"
    // CMCI = "Corrected Machine Check Interrupt"

    LAPIC_REG_LVT_CMCI = 0x2F0,

    // ICR = "Interrupt Command Register"
    LAPIC_REG_ICR_0 = 0x300,
    LAPIC_REG_ICR_1 = 0x310,

    // LVT = "Local Vector Table"
    LAPIC_REG_LVT_TIMER = 0x320,
    LAPIC_REG_LVT_THERMAL = 0x330,
    LAPIC_REG_LVT_PERF_MONITOR_COUNTER = 0x340,
    LAPIC_REG_LVT_LINT0 = 0x350,
    LAPIC_REG_LVT_LINT1 = 0x360,
    LAPIC_REG_LVT_ERROR = 0x370,

    LAPIC_REG_INIT_TIMER_COUNT = 0X380,
    LAPIC_REG_CUR_TIMER_COUNT = 0X390,

    // DVR = "Divide Configuration Register"
    LAPIC_REG_TIMER_DVR = 0x3E0,
};

enum x2apic_lapic_reg {
    X2APIC_LAPIC_REG_ID = 0x2,
    X2APIC_LAPIC_REG_VERSION = 0x3,
    X2APIC_LAPIC_REG_TPR = 0x8,
    X2APIC_LAPIC_REG_PPR = 0xA,
    X2APIC_LAPIC_REG_EOI = 0xB,
    X2APIC_LAPIC_REG_LDR = 0xD,
    X2APIC_LAPIC_REG_SPUR_VECTOR = 0xF,

    X2APIC_LAPIC_REG_ISR0 = 0x10,
    X2APIC_LAPIC_REG_ISR1 = 0x11,
    X2APIC_LAPIC_REG_ISR2 = 0x12,
    X2APIC_LAPIC_REG_ISR3 = 0x13,
    X2APIC_LAPIC_REG_ISR4 = 0x14,
    X2APIC_LAPIC_REG_ISR5 = 0x15,
    X2APIC_LAPIC_REG_ISR6 = 0x16,
    X2APIC_LAPIC_REG_ISR7 = 0x17,

    X2APIC_LAPIC_REG_TMR0 = 0x18,
    X2APIC_LAPIC_REG_TMR1 = 0x19,
    X2APIC_LAPIC_REG_TMR2 = 0x1A,
    X2APIC_LAPIC_REG_TMR3 = 0x1B,
    X2APIC_LAPIC_REG_TMR4 = 0x1C,
    X2APIC_LAPIC_REG_TMR5 = 0x1D,
    X2APIC_LAPIC_REG_TMR6 = 0x1E,
    X2APIC_LAPIC_REG_TMR7 = 0x1F,

    X2APIC_LAPIC_REG_IRR0 = 0x20,
    X2APIC_LAPIC_REG_IRR1 = 0x21,
    X2APIC_LAPIC_REG_IRR2 = 0x22,
    X2APIC_LAPIC_REG_IRR3 = 0x23,
    X2APIC_LAPIC_REG_IRR4 = 0x24,
    X2APIC_LAPIC_REG_IRR5 = 0x25,
    X2APIC_LAPIC_REG_IRR6 = 0x26,
    X2APIC_LAPIC_REG_IRR7 = 0x27,

    X2APIC_LAPIC_REG_ESR = 0x28,
    X2APIC_LAPIC_REG_LVT_CMCI = 0x2F,
    X2APIC_LAPIC_REG_ICR = 0x30,

    X2APIC_LAPIC_REG_LVT_TIMER = 0x32,
    X2APIC_LAPIC_REG_LVT_THERMAL = 0x33,
    X2APIC_LAPIC_REG_LVT_PERF_MONITOR = 0x34,
    X2APIC_LAPIC_REG_LVT_LINT0 = 0x35,
    X2APIC_LAPIC_REG_LVT_LINT1 = 0x36,
    X2APIC_LAPIC_REG_LVT_ERROR = 0x37,

    X2APIC_LAPIC_REG_TIMER_INIT_COUNT = 0x38,
    X2APIC_LAPIC_REG_TIMER_CURR_COUNT = 0x39,

    X2APIC_LAPIC_REG_TIMER_DIVIDE_CONFIG = 0x3E,
    X2APIC_LAPIC_REG_SELF_IPI = 0x3F,
};

enum lapic_ipi {
    LAPIC_IPI_INIT = 0x4500,

    // Send IPI to all APs
    LAPIC_IPI_SIPI = 0x4600,
};

enum lapic_spurious_vector_flags {
    __LAPIC_SPURVEC_ENABLE         = 1 << 8,
    __LAPIC_SPURVEC_FOCUS_DISABLED = 1 << 9,
    __LAPIC_SPURVEC_EOI            = 1 << 11,
};

enum lapic_timer_mode {
    /*
     * In one-shot mode, the timer is started by writing to the initial-count
     * register.
     *
     * Countdown begins after current-count register is set to the init-count
     * register. An interrupt is generated when the timer reaches zero, and
     * the timer remains at its 0 value until reprogrammed.
     */

    LAPIC_TIMER_MODE_ONE_SHOT,

    /*
     * Periodic mode is similar to one-shot mode, except that the when the timer
     * resets back to the value in the init-count register when the timer-value
     * reaches zero.
     */

    LAPIC_TIMER_MODE_PERIODIC,

    /*
     * TSC-Deadline is different. Instead of a countdown, tsc-deadline allows an
     * irq to be generated at an *absolute* time. Both init-count and
     * current-count registers are ignored.
     *
     * The absolute time can be set by writing to the IA32_TSC_DEADLINE MSR.
     * Writing to the msr will start the timer, and the timer will stop after
     * the time has been reached and the interrupt has been generated. In
     * addition, the msr will also be cleared.
     *
     * NOTE: Writing 0 to the msr at *any time* will stop any current timer.
     */

    LAPIC_TIMER_MODE_TSC_DEADLINE
};

struct lapic_info {
    uint8_t apic_id;
    uint8_t processor_id;

    bool enabled : 1;
    bool online_capable : 1;
};

extern volatile struct lapic_registers *lapic_regs;

void lapic_init();
void lapic_add(const struct lapic_info *info);

uint32_t lapic_read(enum x2apic_lapic_reg reg);
void lapic_write(enum x2apic_lapic_reg reg, const uint64_t val);

void lapic_eoi();
void lapic_enable();
void lapic_send_ipi(uint32_t lapic_id, uint32_t vector);
void lapic_send_self_ipi(uint32_t vector);

void lapic_timer_stop();