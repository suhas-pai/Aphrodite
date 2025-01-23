/*
 * kernel/src/acpi/mcfg.c
 * © suhas pai
 */

#include "dev/pci/ecam.h"

#include "acpi/bus.h"
#include "acpi/mcfg.h"

#include "dev/printk.h"

void mcfg_init(const struct os_acpi_mcfg *const mcfg) {
    const uint32_t length = mcfg->sdt.length - sizeof(*mcfg);
    const uint32_t entry_count = length / sizeof(struct os_acpi_mcfg_entry);

    uint32_t index = 0;
    ptrarr_foreach(mcfg->entries, entry_count, entry) {
        printk(LOGLEVEL_INFO,
               "mcfg: pci-group #%" PRIu32 ": mmio at %p, first "
               "bus=%" PRIu32 ", end bus=%" PRIu32 ", segment: %" PRIu32 "\n",
               index + 1,
               (void *)entry->base_addr,
               entry->bus_start_num,
               entry->bus_end_num,
               entry->segment_num);

        const struct range bus_range =
            range_create_end(entry->bus_start_num, entry->bus_end_num);

        struct pci_domain_ecam *const ecam =
            pci_add_ecam_domain(bus_range,
                                /*bus=*/acpi_bus(),
                                entry->base_addr,
                                entry->segment_num);

        bus_probe(&ecam->domain.bus);
        index++;
    }
}