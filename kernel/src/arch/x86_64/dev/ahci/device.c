/*
 * kernel/src/arch/x86_64/dev/ahci/device.c
 * © suhas pai
 */

#include "device.h"

static struct ahci_hba_device g_device = {
    .pci_entity = NULL,
    .regs = NULL,

    .port_list = NULL,

    .supports_64bit_dma = false,
    .supports_staggered_spinup = false,
};

__debug_optimize(3) struct ahci_hba_device *ahci_hba_get() {
    return &g_device;
}
