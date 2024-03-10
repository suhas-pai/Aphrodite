/*
 * kernel/src/arch/x86_64/dev/ahci/port.h"
 * © suhas pai
 */

#pragma once
#include "dev/scsi/request.h"

#include "mm/mmio.h"
#include "sched/event.h"

#include "structs.h"

struct ahci_hba_port_cmdhdr_info {
    struct event event;
    struct await_result result;
};

#define AHCI_HBA_PORT_CMDHDR_INFO_INIT() \
    ((struct ahci_hba_port_cmdhdr_info){ .event = EVENT_INIT() })

enum ahci_hba_port_state {
    AHCI_HBA_PORT_STATE_OK,
    AHCI_HBA_PORT_STATE_NEEDS_RESET,
};

struct ahci_hba_port {
    volatile struct ahci_spec_hba_port *spec;
    volatile struct ahci_spec_port_cmdhdr *headers;

    struct ahci_hba_port_cmdhdr_info *cmdhdr_info_list;

    struct mmio_region *mmio;
    struct spinlock lock;

    uint32_t ports_bitset;
    uint8_t index;

    bool result : 1;
    enum ahci_hba_port_state state : 1;

    union {
        struct {
            uint32_t serr;
            uint32_t interrupt_status;
        } error;
    };

    uint64_t cmdtable_phys;
    enum sata_sig sig;
};

enum ahci_hba_port_command_kind {
    AHCI_HBA_PORT_CMDKIND_READ,
    AHCI_HBA_PORT_CMDKIND_WRITE
};

bool ahci_spec_hba_port_init(struct ahci_hba_port *port);

bool ahci_hba_port_start(struct ahci_hba_port *port);
bool ahci_hba_port_stop(struct ahci_hba_port *port);

bool
ahci_hba_port_send_scsi_command(struct ahci_hba_port *port,
                                struct scsi_request quest,
                                uint64_t phys_addr);
