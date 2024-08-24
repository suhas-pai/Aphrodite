/*
 * kernel/src/arch/x86_64/apic/init.c
 * © suhas pai
 */

#include "acpi/api.h"
#include "apic/lapic.h"

#include "asm/msr.h"
#include "cpu/info.h"
#include "dev/printk.h"

#include "mm/mmio.h"
#include "sys/pic.h"

enum {
    __IA32_MSR_APIC_BASE_IS_BSP = 1 << 8,
    __IA32_MSR_APIC_BASE_X2APIC = 1 << 10,
    __IA32_MSR_APIC_BASE_ENABLE = 1 << 11
};

static struct mmio_region *g_lapic_region = NULL;
static  volatile struct lapic_registers *lapic_regs;

void apic_init(const uint64_t local_apic_base) {
    g_lapic_region =
        vmap_mmio(RANGE_INIT(local_apic_base, PAGE_SIZE),
                  PROT_READ | PROT_WRITE,
                  /*flags=*/0);

    assert_msg(g_lapic_region != NULL,
               "apic: failed to mmio-map local-apic registers");

    lapic_regs = g_lapic_region->base;
    pic_disable();

    printk(LOGLEVEL_INFO, "apic: local apic id: %x\n", lapic_regs->id);
    printk(LOGLEVEL_INFO,
           "apic: local apic version reg: %" PRIx32 ", version: %" PRIu32 "\n",
           lapic_regs->version,
           lapic_regs->version & __LAPIC_VERSION_REG_VERION_MASK);

    uint64_t apic_msr = msr_read(IA32_MSR_APIC_BASE);

    printk(LOGLEVEL_INFO, "apic: msr: 0x%" PRIx64 "\n", apic_msr);
    assert_msg(apic_msr & __IA32_MSR_APIC_BASE_IS_BSP, "apic: cpu is not bsp");

    // Use x2apic if available
    if (get_cpu_capabilities()->supports_x2apic) {
        apic_msr |= __IA32_MSR_APIC_BASE_X2APIC;
        get_acpi_info_mut()->using_x2apic = true;
    } else {
        printk(LOGLEVEL_INFO,
               "apic: x2apic not supported. reverting to xapic instead\n");
    }

    msr_write(IA32_MSR_APIC_BASE, apic_msr | __IA32_MSR_APIC_BASE_ENABLE);
    lapic_init();
}