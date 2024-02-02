/*
 * kernel/src/arch/x86_64/dev/ahci/pio.h"
 * © suhas pai
 */

#pragma once

#include "dev/ata/defines.h"

#include "mm/mmio.h"
#include "sched/event.h"

#include "structs.h"

struct ahci_hba_port {
    struct ahci_hba_device *device;

    volatile struct ahci_spec_hba_port *spec;
    volatile struct ahci_spec_port_cmdhdr *headers;

    struct mmio_region *mmio;
    struct event event;

    uint64_t cmdtable_phys;
    uint8_t index;

    enum sata_sig sig;
};

enum ahci_hba_port_command_kind {
    AHCI_HBA_PORT_CMDKIND_READ,
    AHCI_HBA_PORT_CMDKIND_WRITE
};

void ahci_spec_hba_port_init(struct ahci_hba_port *port);

bool ahci_hba_port_start(struct ahci_hba_port *port);
bool ahci_hba_port_stop(struct ahci_hba_port *port);

void ahci_hba_port_flush_writes(struct ahci_hba_port *port);

bool
ahci_hba_port_send_command(struct ahci_hba_port *port,
                           enum ahci_hba_port_command_kind command_kind,
                           uint64_t sector,
                           uint16_t count,
                           uint64_t response_phys,
                           struct await_result *result_out);

bool
ahci_hba_port_send_ata_command(struct ahci_hba_port *port,
                               enum ata_command command,
                               uint64_t sector,
                               uint16_t count,
                               uint64_t phys_addr,
                               struct await_result *result_out);
