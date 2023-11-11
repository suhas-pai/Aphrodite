/*
 * kernel/src/arch/x86_64/dev/ahci/pio.h"
 * © suhas pai
 */

#pragma once
#include "structs.h"

struct ahci_hba_port {
    struct ahci_spec_hba_port *const spec;
    struct ahci_spec_port_cmd_header *headers;

    uint8_t free_count : 5;
};

enum ahci_hba_port_command_kind {
    AHCI_HBA_PORT_CMDKIND_READ,
    AHCI_HBA_PORT_CMDKIND_WRITE
};

void
ahci_hba_port_send_command(struct ahci_hba_port *port,
                           enum ahci_hba_port_command_kind command_kind,
                           uint64_t sector,
                           uint16_t count,
                           uint8_t slot);