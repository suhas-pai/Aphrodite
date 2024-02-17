/*
 * kernel/src/arch/aarch64/acpi/gtdt.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/util.h"
#include "sys/gic.h"

#include "gtdt.h"

void gtdt_init(const struct acpi_gtdt *const gtdt) {
    printk(LOGLEVEL_INFO,
           "gtdt:\n"
           "\tctrl base phys address: 0x%" PRIx64 "\n"
           "\tsecure el1 timer gsiv: %" PRIu32 "\n"
           "\tsecure el1 timer flags: %" PRIu32 "\n"
           "\tnon-secure el1 timer gsiv: %" PRIu32 "\n"
           "\tnon-secure el1 timer flags: %" PRIu32 "\n"
           "\tvirtual el1 timer gsiv: %" PRIu32 "\n"
           "\tvirtual el1 timer flags: %" PRIu32 "\n"
           "\tel2 timer gsiv: %" PRIu32 "\n"
           "\tel2 timer flags: %" PRIu32 "\n"
           "\tread base phys address: 0x%" PRIx64 "\n"
           "\tplatform timer count: %" PRIu32 "\n"
           "\tplatform timer offset: 0x%" PRIx32 "\n"
           "\tvirtual el2 timer gsiv: %" PRIu32 "\n"
           "\tvirtual el2 timer flags: 0x%" PRIx32 "\n",
           gtdt->ctrl_base_phys_address,
           gtdt->secure_el1_timer_gsiv,
           gtdt->secure_el1_timer_flags,
           gtdt->non_secure_el1_timer_gsiv,
           gtdt->non_secure_el1_timer_flags,
           gtdt->virtual_el1_timer_gsiv,
           gtdt->virtual_el1_timer_flags,
           gtdt->el2_timer_gsiv,
           gtdt->el2_timer_flags,
           gtdt->read_base_phys_address,
           gtdt->platform_timer_count,
           gtdt->platform_timer_offset,
           gtdt->virtual_el2_timer_gsiv,
           gtdt->virtual_el2_timer_flags);

    const enum irq_trigger_mpde secure_el1_trigger_mode =
        (gtdt->secure_el1_timer_flags & __ACPI_GTDT_EDGE_TRIGGER_IRQ) ?
            IRQ_TRIGGER_MODE_EDGE : IRQ_TRIGGER_MODE_LEVEL;

    const enum irq_trigger_mpde non_secure_el1_trigger_mode =
        (gtdt->non_secure_el1_timer_flags & __ACPI_GTDT_EDGE_TRIGGER_IRQ) ?
            IRQ_TRIGGER_MODE_EDGE : IRQ_TRIGGER_MODE_LEVEL;

    const enum irq_trigger_mpde virtual_el1_trigger_mode =
        (gtdt->virtual_el1_timer_flags & __ACPI_GTDT_EDGE_TRIGGER_IRQ) ?
            IRQ_TRIGGER_MODE_EDGE : IRQ_TRIGGER_MODE_LEVEL;

    gicd_set_irq_trigger_mode(gtdt->secure_el1_timer_gsiv,
                              secure_el1_trigger_mode);

    gicd_set_irq_trigger_mode(gtdt->non_secure_el1_timer_gsiv,
                              non_secure_el1_trigger_mode);

    gicd_set_irq_trigger_mode(gtdt->virtual_el1_timer_gsiv,
                              virtual_el1_trigger_mode);

    /*
    const enum irq_trigger_mpde el2_trigger_mode =
        (gtdt->el2_timer_flags & __ACPI_GTDT_EDGE_TRIGGER_IRQ) ?
            IRQ_TRIGGER_MODE_EDGE : IRQ_TRIGGER_MODE_LEVEL;

    gicd_set_irq_trigger_mode(gtdt->el2_timer_gsiv, el2_trigger_mode);
    */

    gicd_unmask_irq(gtdt->secure_el1_timer_gsiv);
    gicd_unmask_irq(gtdt->non_secure_el1_timer_gsiv);
    gicd_unmask_irq(gtdt->virtual_el1_timer_gsiv);

    if (!index_in_bounds(gtdt->platform_timer_offset, gtdt->sdt.length)) {
        printk(LOGLEVEL_WARN,
               "gtdt: platform-timer offset is beyond end of structure\n");
        return;
    }
}