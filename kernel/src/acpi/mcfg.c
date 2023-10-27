/*
 * kernel/acpi/mcfg.c
 * © suhas pai
 */

#include "dev/pci/pci.h"
#include "dev/printk.h"

#include "mcfg.h"

void mcfg_init(const struct acpi_mcfg *const mcfg) {
    const uint32_t length = mcfg->sdt.length - sizeof(*mcfg);
    const uint32_t entry_count = length / sizeof(struct acpi_mcfg_entry);

    const struct acpi_mcfg_entry *iter = mcfg->entries;
    const struct acpi_mcfg_entry *const end = &mcfg->entries[entry_count];

    for (uint32_t index = 0; iter != end; iter++, index++) {
        printk(LOGLEVEL_INFO,
               "mcfg: pci-group #%" PRIu32 ": mmio at %p, first "
               "bus=%" PRIu32 ", end bus=%" PRIu32 ", segment: %" PRIu32 "\n",
               index + 1,
               (void *)iter->base_addr,
               iter->bus_start_num,
               iter->bus_end_num,
               iter->segment_num);

        const struct range bus_range =
            range_create_end(iter->bus_start_num, iter->bus_end_num);

        pci_add_pcie_domain(bus_range, iter->base_addr, iter->segment_num);
    }
}