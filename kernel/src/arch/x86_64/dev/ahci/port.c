/*
 * kernel/src/arch/x86_64/dev/ahci/port.c
 * © suhas pai
 */

#include "dev/ata/atapi.h"

#include "dev/printk.h"
#include "lib/size.h"
#include "mm/mm_types.h"
#include "sys/mmio.h"

#include "port.h"

#define AHCI_HBA_PORT_MAX_COUNT mib(4)

void
ahci_hba_port_send_command(struct ahci_hba_port *const port,
                           const enum ahci_hba_port_command_kind command_kind,
                           const uint64_t sector,
                           const uint16_t count,
                           const uint8_t slot)
{
    if (__builtin_expect(count == 0, 0)) {
        printk(LOGLEVEL_WARN, "ahci-port: got request of 0 bytes\n");
        return;
    }

    (void)sector;

    struct ahci_spec_port_cmd_header *const cmd_header = port->headers + slot;
    uint16_t flags =
        mmio_read(&cmd_header->flags) |
        __AHCI_PORT_CMDHDR_CLR_BUSY_ON_ROK |
        __AHCI_PORT_CMDHDR_PREFETCHABLE |
        (sizeof(struct ahci_spec_fis_reg_h2d) / 4);

    if (command_kind == AHCI_HBA_PORT_CMDKIND_WRITE) {
        flags |= __AHCI_PORT_CMDHDR_WRITE;
    } else {
        flags &= (uint16_t)~__AHCI_PORT_CMDHDR_WRITE;
    }

    const uint32_t prdt_count = div_round_up(count, AHCI_HBA_PORT_MAX_COUNT);

    mmio_write(&cmd_header->flags, flags);
    mmio_write(&cmd_header->prdt_length, prdt_count);

    const uint64_t cmd_table_phys =
        (uint64_t)mmio_read(&cmd_header->cmd_table_base_upper32) << 32 |
        mmio_read(&cmd_header->cmd_table_base_lower32);

    struct ahci_spec_hba_cmd_table *const cmd_table =
        phys_to_virt(cmd_table_phys);
    volatile struct ahci_spec_hba_prdt_entry *const entries =
        cmd_table->prdt_entries;

    for (uint8_t i = 0; i < prdt_count - 1; i++) {
        volatile struct ahci_spec_hba_prdt_entry *const entry = &entries[i];
        const uint32_t entry_flags =
            (AHCI_HBA_PORT_MAX_COUNT - 1) |
            __AHCI_SPEC_HBA_PRDT_ENTRY_INT_ON_COMPLETION;

        mmio_write(&entry->flags, entry_flags);
    }

    volatile struct ahci_spec_hba_prdt_entry *const last_entry =
        &entries[prdt_count - 1];

    const uint32_t last_entry_byte_count = count % AHCI_HBA_PORT_MAX_COUNT;
    uint32_t last_entry_flags = 0;

    if (last_entry_byte_count != 0) {
        last_entry_flags =
            (last_entry_byte_count - 1) |
            __AHCI_SPEC_HBA_PRDT_ENTRY_INT_ON_COMPLETION;
    } else {
        last_entry_flags =
            (AHCI_HBA_PORT_MAX_COUNT - 1) |
            __AHCI_SPEC_HBA_PRDT_ENTRY_INT_ON_COMPLETION;
    }

    mmio_write(&last_entry->flags, last_entry_flags);
    struct ahci_spec_fis_reg_h2d *const h2d_fis =
        (struct ahci_spec_fis_reg_h2d *)(uint64_t)cmd_table->command_fis;

    h2d_fis->fis_type = AHCI_FIS_KIND_REG_H2D;
    h2d_fis->flags = 0;
    h2d_fis->command =
        (sector >= (1ull << 48)) ? ATAPI_READ_DMA_EXT : ATAPI_READ_DMA;

    h2d_fis->feature_low8 = 0;
    h2d_fis->control = 0;
    h2d_fis->icc = 0;

    h2d_fis->lba0 = (uint8_t)sector;
    h2d_fis->lba1 = (uint8_t)(sector >> 8);
    h2d_fis->lba2 = (uint8_t)(sector >> 16);

    h2d_fis->device = 1 << 6;

    h2d_fis->lba3 = (uint8_t)(sector >> 24);
    h2d_fis->lba4 = (uint8_t)(sector >> 32);
    h2d_fis->lba5 = (uint8_t)(sector >> 40);

    h2d_fis->feature_high8 = 0;
    h2d_fis->count_low = (uint8_t)count;
    h2d_fis->count_high = (uint8_t)(count >> 8);
    h2d_fis->icc = 0;
    h2d_fis->control = 0;

    port->spec->command_issue |= (1ull << slot);
}