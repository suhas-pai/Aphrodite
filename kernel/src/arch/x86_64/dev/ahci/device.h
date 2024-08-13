/*
 * kernel/src/arch/x86_64/dev/ahci/device.h
 * © suhas pai
 */

#pragma once

#include "port.h"
#include "structs.h"

enum ahci_hba_device_kind {
    AHCI_HBA_DEVICE_KIND_SATA_ATA,
    AHCI_HBA_DEVICE_KIND_SATA_ATAPI,
    AHCI_HBA_DEVICE_KIND_SATA_SEMB,
    AHCI_HBA_DEVICE_KIND_SATA_PM,
};

struct ahci_hba_device {
    struct pci_entity_info *pci_entity;
    struct ahci_hba_port *port_list;

    volatile struct ahci_spec_hba_regs *regs;
    uint8_t port_count;

    bool supports_64bit_dma : 1;
    bool supports_staggered_spinup : 1;
};

struct ahci_hba_device *ahci_hba_get();
bool ahci_hba_reset();
