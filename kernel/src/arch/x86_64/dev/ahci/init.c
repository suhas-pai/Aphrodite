/*
 * kernel/src/dev/ahci/init.c
 * © suhas pai
 */

#include "dev/pci/structs.h"

#include "asm/pause.h"
#include "cpu/isr.h"

#include "dev/driver.h"
#include "dev/printk.h"

#include "lib/bits.h"
#include "lib/util.h"

#include "mm/kmalloc.h"
#include "sys/mmio.h"

#include "device.h"
#include "irq.h"

#define MAX_ATTEMPTS 100
static isr_vector_t g_hba_vector = 0;

__debug_optimize(3) static
bool ahci_hba_probe_port(volatile struct ahci_spec_hba_port *const port) {
    const uint32_t sata_status = mmio_read(&port->sata_status);
    const enum ahci_hba_port_ipm ipm =
        (sata_status & __AHCI_HBA_PORT_SATA_STAT_CTRL_IPM) >>
            AHCI_HBA_PORT_SATA_STAT_CTRL_IPM_SHIFT;

    const enum ahci_hba_port_det det =
        sata_status & __AHCI_HBA_PORT_SATA_STAT_CTRL_DET;

    return det == AHCI_HBA_PORT_DET_PRESENT && ipm == AHCI_HBA_PORT_IPM_ACTIVE;
}

static bool init_with_regs(volatile struct ahci_spec_hba_regs *const regs) {
    const uint32_t version = mmio_read(&regs->version);
    printk(LOGLEVEL_INFO,
           "ahci: version is %" PRIu16 ".%" PRIu16 "\n",
           version >> 16,
           (uint16_t)version);

    struct ahci_hba_device *const hba = ahci_hba_get();
    hba->regs = regs;

    // 1. Indicate that system software is AHCI aware by setting GHC.AE to ‘1’
    mmio_write(&regs->global_host_control,
               __AHCI_HBA_GLOBAL_HOST_CTRL_AHCI_ENABLE);

    // Perform BIOS/OS Handoff.
    const uint32_t host_cap_ext = mmio_read(&regs->host_capabilities_ext);
    if (host_cap_ext & __AHCI_HBA_HOST_CAP_EXT_BIOS_HANDOFF) {
        printk(LOGLEVEL_INFO, "ahci: hba supports bios handoff\n");
        mmio_write(&regs->bios_os_handoff_ctrl_status,
                   mmio_read(&regs->bios_os_handoff_ctrl_status)
                 | __AHCI_HBA_BIOS_HANDOFF_STATUS_CTRL_OS_OWNED_SEM);

        bool handoff_successful = false;
        for (uint32_t i = 0; i != MAX_ATTEMPTS; i++) {
            if ((mmio_read(&regs->bios_os_handoff_ctrl_status)
                    & __AHCI_HBA_BIOS_HANDOFF_STATUS_CTRL_BIOS_BUSY) == 0)
            {
                handoff_successful = true;
                break;
            }

            cpu_pause();
        }

        if (!handoff_successful) {
            printk(LOGLEVEL_WARN, "ahci: bios-os handoff failed\n");
            return false;
        }
    }

    // 2. Determine which ports are implemented by the HBA, by reading the PI
    // register. This bit map value will aid software in determining how many
    // ports are available and which port registers need to be initialized.

    const uint32_t ports_impled = mmio_read(&regs->ports_implemented);
    const uint8_t ports_impled_count =
        count_all_one_bits(ports_impled,
                           /*start_index=*/0,
                           /*end_index=*/sizeof_bits(ports_impled));

    if (__builtin_expect(ports_impled_count == 0, 0)) {
        printk(LOGLEVEL_WARN, "ahci: no ports are implemented\n");
        return false;
    }

    printk(LOGLEVEL_INFO,
           "ahci: has %" PRIu32 " ports implemented\n",
           ports_impled_count);

    hba->port_list = kmalloc(sizeof(struct ahci_hba_port) * ports_impled_count);
    if (hba->port_list == NULL) {
        printk(LOGLEVEL_WARN,
               "ahci: failed to allocate memory for port list\n");

        return false;
    }

    uint8_t usable_port_count = 0;
    for (uint8_t index = 0; index != AHCI_HBA_MAX_PORT_COUNT; index++) {
        if ((ports_impled & 1ull << index) == 0) {
            continue;
        }

        struct ahci_hba_port *const port = &hba->port_list[usable_port_count];

        port->index = index;
        port->spec = &regs->ports[index];

        if (ahci_hba_port_is_idle(port)) {
            // We haven't checked if this port is actually usable, but we can't,
            // so we check it later instead.

            usable_port_count++;
            continue;
        }

        // If the port is running, we can check the signature to use if anything
        // is connected to it.

        if (ahci_hba_probe_port(port->spec)) {
            usable_port_count++;
        }

        if (!ahci_hba_port_stop(port)) {
            kfree(hba->port_list);
            return false;
        }
    }

    if (usable_port_count == 0) {
        kfree(hba->port_list);
        printk(LOGLEVEL_WARN,
               "ahci: no implemented ports are both present and active\n");

        return false;
    }

    hba->port_count = usable_port_count;

    // Now that we've ensured all ports are idle, fully reset the controller and
    // re-init.

    if (!ahci_hba_reset()) {
        kfree(hba->port_list);
        printk(LOGLEVEL_WARN,
               "ahci: failed to reset controller before full init\n");

        return false;
    }

    mmio_write(&regs->global_host_control,
               __AHCI_HBA_GLOBAL_HOST_CTRL_AHCI_ENABLE);
    mmio_write(&regs->global_host_control,
               __AHCI_HBA_GLOBAL_HOST_CTRL_INTR_ENABLE);

    // 4. Determine how many command slots the HBA supports, by reading CAP.NCS.
    const uint64_t host_cap = mmio_read(&regs->host_capabilities);

    hba->supports_64bit_dma = host_cap & __AHCI_HBA_HOST_CAP_64BIT_DMA;
    hba->supports_staggered_spinup =
        host_cap & __AHCI_HBA_HOST_CAP_SUPPORTS_STAGGERED_SPINUP;

    if (hba->supports_64bit_dma) {
        printk(LOGLEVEL_INFO, "ahci: hba supports 64-bit dma\n");
    } else {
        printk(LOGLEVEL_WARN, "ahci: hba doesn't support 64-bit dma\n");
    }

    if (hba->supports_staggered_spinup) {
        printk(LOGLEVEL_INFO, "ahci: hba supports staggered spinup\n");
    } else {
        printk(LOGLEVEL_INFO, "ahci: hba doesn't support staggered spinup\n");
    }

    bool init_one_port = false;
    for (uint8_t index = 0; index != usable_port_count; index++) {
        if (ahci_spec_hba_port_init(&hba->port_list[index])) {
            init_one_port = true;
        }
    }

    if (!init_one_port) {
        kfree(hba->port_list);
        printk(LOGLEVEL_WARN,
               "ahci: no implemented port is usable. aborting init\n");

        return false;
    }

    printk(LOGLEVEL_INFO, "ahci: fully initialized\n");
    return true;
}

static void init_from_pci(struct pci_entity_info *const pci_entity) {
    if (pci_entity->msi_support == PCI_ENTITY_MSI_SUPPORT_NONE) {
        printk(LOGLEVEL_WARN, "ahci: doesn't support msi[x]. skipping init\n");
        return;
    }

    if (!index_in_bounds(AHCI_HBA_REGS_BAR_INDEX, pci_entity->max_bar_count)) {
        printk(LOGLEVEL_WARN,
               "ahci: pci-device has fewer than %" PRIu32 " bars\n",
               AHCI_HBA_REGS_BAR_INDEX);
        return;
    }

    struct pci_entity_bar_info *const bar =
        &pci_entity->bar_list[AHCI_HBA_REGS_BAR_INDEX];

    if (!bar->is_present) {
        printk(LOGLEVEL_WARN,
               "ahci: pci-device doesn't have the required bar at "
               "index %" PRIu32 "\n",
               AHCI_HBA_REGS_BAR_INDEX);
        return;
    }

    if (!bar->is_mmio) {
        printk(LOGLEVEL_WARN,
               "ahci: pci-device's bar at index %" PRIu32 " isn't an mmio "
               "bar\n",
               AHCI_HBA_REGS_BAR_INDEX);
        return;
    }

    if (!pci_map_bar(bar)) {
        printk(LOGLEVEL_WARN,
               "ahci: failed to map pci bar at index %" PRIu32 "\n",
               AHCI_HBA_REGS_BAR_INDEX);
        return;
    }

    pci_entity_enable_privls(pci_entity,
                             __PCI_ENTITY_PRIVL_BUS_MASTER
                           | __PCI_ENTITY_PRIVL_MEM_ACCESS);

    g_hba_vector = isr_alloc_msi_vector(&pci_entity->device, /*msi_index=*/0);
    assert(g_hba_vector != ISR_INVALID_VECTOR);

    isr_set_vector(g_hba_vector, ahci_port_handle_irq, &ARCH_ISR_INFO_NONE());
    pci_entity_enable_msi(pci_entity);

    with_preempt_disabled({
        pci_entity_bind_msi_to_vector(pci_entity,
                                      this_cpu(),
                                      g_hba_vector,
                                      /*masked=*/false);
    });

    volatile struct ahci_spec_hba_regs *const regs =
        pci_entity_bar_get_base(bar);

    struct ahci_hba_device *const hba = ahci_hba_get();
    hba->pci_entity = pci_entity;

    if (!init_with_regs(regs)) {
        isr_free_msi_vector(&pci_entity->device, g_hba_vector, /*msi_index=*/0);
        pci_entity_toggle_msi_vector_mask(pci_entity,
                                          g_hba_vector,
                                          /*mask=*/true);

        pci_entity_disable_privls(pci_entity);
        pci_unmap_bar(bar);
    }
}

static const struct pci_driver pci_driver = {
    .vendor = 0x1af4,
    .class = PCI_ENTITY_CLASS_MASS_STORAGE_CONTROLLER,
    .subclass = PCI_ENTITY_SUBCLASS_SATA,
    .prog_if = 0x1,
    .match =
        __PCI_DRIVER_MATCH_CLASS
      | __PCI_DRIVER_MATCH_SUBCLASS
      | __PCI_DRIVER_MATCH_PROGIF,
    .init = init_from_pci
};

__driver static const struct driver driver = {
    .name = SV_STATIC("x86_64-ahci-driver"),
    .dtb = NULL,
    .pci = &pci_driver
};