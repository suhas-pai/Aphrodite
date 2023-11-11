/*
 * kernel/src/arch/x86_64/dev/ahci/port.c
 * © suhas pai
 */

#include "mm/mm_types.h"
#include "sys/mmio.h"

#include "port.h"

void
ahci_hba_port_send_command(struct ahci_hba_port *const port,
                           const enum ahci_hba_port_command_kind command_kind,
                           const uint64_t sector,
                           const uint16_t count,
                           const uint8_t slot)
{
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

    mmio_write(&cmd_header->flags, flags);
    mmio_write(&cmd_header->prd_byte_count, ((count - 1) >> 4) + 1);

#if 0
    const uint64_t cmd_table_phys =
        (uint64_t)mmio_read(&cmd_header->cmd_table_base_upper32) << 32 |
        mmio_read(&cmd_header->cmd_table_base_lower32);

    struct ahci_spec_hba_prdt_entry *const entries =
        phys_to_virt(cmd_table_phys);

    for (uint8_t i = 0; i != count - 1; i++) {
        struct ahci_spec_hba_prdt_entry *const entry = entries + i;
    }
#endif
}