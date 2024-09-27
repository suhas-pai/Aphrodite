/*
 * kernel/src/arch/x86_64/dev/ahci/device.c
 * © suhas pai
 */

#include "sys/mmio.h"
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

#define MAX_ATTEMPTS 100

bool ahci_hba_reset() {
    mmio_write(&g_device.regs->global_host_control,
               __AHCI_HBA_GLOBAL_HOST_CTRL_HBA_RESET);

    for (int i = 0; i != MAX_ATTEMPTS; i++) {
        const uint32_t ghc = mmio_read(&g_device.regs->global_host_control);
        if ((ghc & __AHCI_HBA_GLOBAL_HOST_CTRL_HBA_RESET) == 0) {
            return true;
        }
    }

    return false;
}
