/*
 * kernel/src/dev/ahci/init.c
 * © suhas pai
 */

#include "dev/ata/defines.h"
#include "asm/pause.h"

#include "dev/driver.h"
#include "dev/pci/structs.h"
#include "dev/printk.h"

#include "lib/bits.h"
#include "lib/util.h"

#include "mm/kmalloc.h"
#include "mm/zone.h"

#include "sys/mmio.h"
#include "port.h"

struct ahci_device {
    struct pci_entity_info *device;
    volatile struct ahci_spec_hba_registers *regs;

    struct ahci_hba_port *port_list;

    bool supports_64bit_dma : 1;
    bool supports_staggered_spinup : 1;
};

__optimize(3) static void
ahci_hba_port_power_on_and_spin_up(
    volatile struct ahci_spec_hba_port *const port,
    struct ahci_device *const device,
    const uint8_t index)
{
    const uint32_t cmd_status = mmio_read(&port->command_and_status);
    if (cmd_status & __AHCI_HBA_PORT_CMDSTATUS_COLD_PRESENCE_DETECTION) {
        mmio_write(&port->command_and_status,
                   cmd_status | __AHCI_HBA_PORT_CMDSTATUS_POWERON_DEVICE);

        printk(LOGLEVEL_INFO,
               "ahci: powering on port #%" PRIu8 "\n",
               index + 1);
    } else {
        printk(LOGLEVEL_WARN,
               "ahci: port #%" PRIu8 " has no cold-presence detection\n",
               index + 1);
    }

    if (device->supports_staggered_spinup) {
        mmio_write(&port->command_and_status,
                   cmd_status | __AHCI_HBA_PORT_CMDSTATUS_SPINUP_DEVICE);

        printk(LOGLEVEL_INFO,
               "ahci: spinning up port #%" PRIu8 "\n",
               index + 1);
    }
}

__optimize(3) static void
ahci_hba_port_set_state(volatile struct ahci_spec_hba_port *const port,
                        const enum ahci_hba_port_interface_comm_ctrl ctrl)
{
    uint32_t cmd_status = mmio_read(&port->command_and_status);

    cmd_status &= ~__AHCI_HBA_PORT_CMDSTATUS_INTERFACE_COMM_CTRL;
    cmd_status |=
        ctrl << AHCI_HBA_PORT_CMDSTATUS_INTERFACE_COMM_CTRL_SHIFT;

    mmio_write(&port->command_and_status, cmd_status);
}

#define MAX_ATTEMPTS 10

__optimize(3) static bool
ahci_hba_port_start_running(volatile struct ahci_spec_hba_port *const port,
                            const uint8_t index)
{
    const uint32_t flags =
        __AHCI_HBA_PORT_CMDSTATUS_FIS_RECEIVE_ENABLE |
        __AHCI_HBA_PORT_CMDSTATUS_START;

    mmio_write(&port->command_and_status,
               mmio_read(&port->command_and_status) | flags);

    const uint32_t tfd_flags = __ATA_STATUS_REG_BSY | __ATA_STATUS_REG_DRQ;
    for (uint8_t i = 0; i != MAX_ATTEMPTS; i++) {
        if ((mmio_read(&port->task_file_data) & tfd_flags) == 0) {
            printk(LOGLEVEL_WARN,
                   "ahci: successfully initialized port #%" PRIu8 "\n",
                   index + 1);
            return true;
        }

        cpu_pause();
    }

    printk(LOGLEVEL_WARN,
           "ahci: failed to initialize port #%" PRIu8 ", spinup taking too "
           "long\n",
           index + 1);

    return false;
}

__optimize(3) static bool
ahci_hba_port_stop_running(volatile struct ahci_spec_hba_port *const port) {
    const uint32_t flags =
        __AHCI_HBA_PORT_CMDSTATUS_FIS_RECEIVE_ENABLE |
        __AHCI_HBA_PORT_CMDSTATUS_START;

    mmio_write(&port->command_and_status,
               mmio_read(&port->command_and_status) & ~flags);

    for (uint8_t i = 0; i != MAX_ATTEMPTS; i++) {
        if ((mmio_read(&port->command_and_status) & flags) == 0) {
            return true;
        }

        cpu_pause();
    }

    return false;
}

#define AHCI_HBA_CMD_TABLE_PAGE_ORDER 1

_Static_assert(
    ((uint64_t)sizeof(struct ahci_spec_hba_cmd_table) *
     // Each port (upto 32 exist) has a separate command-table
     sizeof_bits_field(struct ahci_spec_hba_registers, port_implemented))
        <= (PAGE_SIZE << AHCI_HBA_CMD_TABLE_PAGE_ORDER),
    "AHCI_HBA_CMD_TABLE_PAGE_ORDER is too low to fit all "
    "struct ahci_port_command_header entries");

__optimize(3) static void
ahci_spec_hba_port_init(struct ahci_hba_port *const port,
                        volatile struct ahci_spec_hba_port *const spec,
                        const uint8_t index,
                        struct ahci_device *const device)
{
    if (!ahci_hba_port_stop_running(spec)) {
        printk(LOGLEVEL_WARN,
               "ahci: failed to stop port #%" PRIu8 " before init\n",
               index + 1);
        return;
    }

    struct page *cmd_list_page = NULL;
    struct page *cmd_table_pages = NULL;

    if (device->supports_64bit_dma) {
        cmd_list_page = alloc_page(PAGE_STATE_USED, __ALLOC_ZERO);
        cmd_table_pages =
            alloc_pages(PAGE_STATE_USED,
                        __ALLOC_ZERO,
                        AHCI_HBA_CMD_TABLE_PAGE_ORDER);
    } else {
        cmd_list_page =
            alloc_pages_from_zone(page_zone_low4g(),
                                  PAGE_STATE_USED,
                                  __ALLOC_ZERO,
                                  /*order=*/0,
                                  /*allow_fallback=*/true);
        cmd_table_pages =
            alloc_pages_from_zone(page_zone_low4g(),
                                  PAGE_STATE_USED,
                                  __ALLOC_ZERO,
                                  AHCI_HBA_CMD_TABLE_PAGE_ORDER,
                                  /*allow_fallback=*/true);
    }

    if (cmd_list_page == NULL) {
        if (cmd_table_pages != NULL) {
            free_pages(cmd_table_pages, AHCI_HBA_CMD_TABLE_PAGE_ORDER);
        }

        printk(LOGLEVEL_WARN, "ahci: failed to allocate page for cmd-table\n");
        return;
    }

    if (cmd_table_pages == NULL) {
        free_page(cmd_list_page);
        printk(LOGLEVEL_WARN, "ahci: failed to allocate pages for hba port\n");

        return;
    }

    const struct range phys_range =
        RANGE_INIT(page_to_phys(cmd_table_pages),
                   PAGE_SIZE << AHCI_HBA_CMD_TABLE_PAGE_ORDER);

    const uint64_t cmd_list_phys = page_to_phys(cmd_list_page);

    mmio_write(&spec->cmd_list_base_phys_lower32, cmd_list_phys);
    mmio_write(&spec->cmd_list_base_phys_upper32, cmd_list_phys >> 32);

    printk(LOGLEVEL_INFO,
           "ahci: port #%" PRIu8 " has a cmd-list base at %p\n",
           index + 1,
           (void *)phys_range.front);

    volatile struct ahci_spec_port_cmd_header *cmd_header =
        phys_to_virt(phys_range.front);
    const uint8_t total_port_count =
        sizeof_bits_field(struct ahci_spec_hba_registers, port_implemented);
    const volatile struct ahci_spec_port_cmd_header *const end =
        cmd_header + total_port_count;

    for (uint64_t phys = phys_range.front; cmd_header != end; cmd_header++) {
        mmio_write(&cmd_header->flags, __AHCI_PORT_CMDHDR_PREFETCHABLE);
        mmio_write(&cmd_header->prdt_length, AHCI_HBA_MAX_PRDT_ENTRIES);
        mmio_write(&cmd_header->prd_byte_count, 0);
        mmio_write(&cmd_header->cmd_table_base_lower32, phys);

        if (device->supports_64bit_dma) {
            mmio_write(&cmd_header->cmd_table_base_upper32, phys >> 32);
        }

        phys += sizeof(*cmd_header);
    }

    ahci_hba_port_power_on_and_spin_up(spec, device, index);
    ahci_hba_port_set_state(spec, AHCI_HBA_PORT_INTERFACE_COMM_CTRL_ACTIVE);

    if (!ahci_hba_port_start_running(spec, index)) {
        free_page(cmd_list_page);
        free_pages(cmd_table_pages, AHCI_HBA_CMD_TABLE_PAGE_ORDER);

        return;
    }

    struct mmio_region *const mmio =
        vmap_mmio(phys_range, PROT_READ | PROT_WRITE, /*flags=*/0);

    if (mmio == NULL) {
        free_page(cmd_list_page);
        free_pages(cmd_table_pages, AHCI_HBA_CMD_TABLE_PAGE_ORDER);

        return;
    }

    port->cmdlist_phys = page_to_phys(cmd_list_page);
    port->cmdtable_phys = page_to_phys(cmd_table_pages);
    port->index = index;
    port->mmio = mmio;

    mmio_write(&spec->interrupt_enable,
               __AHCI_HBA_IE_DEV_TO_HOST_FIS_INT_ENABLE |
               __AHCI_HBA_IE_PIO_SETUP_FIS_INT_ENABLE |
               __AHCI_HBA_IE_DMA_SETUP_FIS_INT_ENABLE |
               __AHCI_HBA_IE_SET_DEV_BITS_FIS_INT_ENABLE |
               __AHCI_HBA_IE_UNKNOWN_FIS_INT_ENABLE |
               __AHCI_HBA_IE_DESC_PROCESSED_INT_ENABLE |
               __AHCI_HBA_IE_PORT_CHANGE_INT_ENABLE |
               __AHCI_HBA_IE_DEV_MECH_PRESENCE_INT_ENABLE |
               __AHCI_HBA_IE_PHYRDY_CHANGE_STATUS |
               __AHCI_HBA_IE_INCORRECT_PORT_MULT_STATUS |
               __AHCI_HBA_IE_OVERFLOW_STATUS |
               __AHCI_HBA_IE_INTERFACE_NOT_FATAL_ERR_STATUS |
               __AHCI_HBA_IE_INTERFACE_FATAL_ERR_STATUS |
               __AHCI_HBA_IE_HOST_BUS_DATA_ERR_STATUS |
               __AHCI_HBA_IE_HOST_BUS_FATAL_ERR_STATUS |
               __AHCI_HBA_IE_TASK_FILE_ERR_STATUS |
               __AHCI_HBA_IE_COLD_PORT_DETECT_STATUS);
}

__optimize(3) static
bool ahci_hba_probe_port(volatile struct ahci_spec_hba_port *const port) {
    const uint32_t sata_status = mmio_read(&port->sata_status);

    const enum ahci_hba_port_ipm ipm = (sata_status >> 8) & 0x0F;
    const enum ahci_hba_port_det det = sata_status & 0x0F;

    return det == AHCI_HBA_PORT_DET_PRESENT && ipm == AHCI_HBA_PORT_IPM_ACTIVE;
}

static void init_from_pci(struct pci_entity_info *const pci_entity) {
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

    pci_entity_enable_privl(pci_entity,
                            __PCI_ENTITY_PRIVL_BUS_MASTER |
                            __PCI_ENTITY_PRIVL_MEM_ACCESS);

    volatile struct ahci_spec_hba_registers *const regs =
        (volatile struct ahci_spec_hba_registers *)bar->mmio->base;

    const uint32_t version = mmio_read(&regs->version);
    printk(LOGLEVEL_INFO,
           "ahci: version is %" PRIu32 ".%" PRIu32 "\n",
           version >> 16,
           (uint16_t)version);

    const uint32_t ports_impled = mmio_read(&regs->port_implemented);
    const uint8_t ports_impled_count =
        count_all_one_bits(ports_impled,
                           /*start_index=*/0,
                           /*end_index=*/sizeof_bits(ports_impled));

    if (__builtin_expect(ports_impled_count == 0, 0)) {
        printk(LOGLEVEL_WARN, "ahci: no ports are implemented\n");
        return;
    }

    printk(LOGLEVEL_INFO,
           "ahci: has %" PRIu32 " ports implemented\n",
           ports_impled_count);

    const uint64_t host_cap = mmio_read(&regs->host_capabilities);
    struct ahci_device device = {
        .device = pci_entity,
        .regs = regs,
        .port_list =
            kmalloc(sizeof(struct ahci_hba_port) * ports_impled),
        .supports_64bit_dma = host_cap & __AHCI_HBA_HOST_CAP_64BIT_DMA,
        .supports_staggered_spinup =
            host_cap & __AHCI_HBA_HOST_CAP_SUPPORTS_STAGGERED_SPINUP,
    };

    if (device.port_list == NULL) {
        printk(LOGLEVEL_WARN,
               "ahci: failed to allocate memory for port list\n");
        return;
    }

    if (device.supports_64bit_dma) {
        printk(LOGLEVEL_INFO, "ahci: hba supports 64-bit dma\n");
    } else {
        printk(LOGLEVEL_WARN, "ahci: hba doesn't support 64-bit dma\n");
    }

    if (device.supports_staggered_spinup) {
        printk(LOGLEVEL_INFO, "ahci: hba supports staggered spinup\n");
    } else {
        printk(LOGLEVEL_INFO, "ahci: hba doesn't support staggered spinup\n");
    }

    uint8_t usable_port_count = 0;
    for (uint8_t index = 0; index != sizeof_bits(ports_impled); index++) {
        if ((ports_impled & (1ull << index)) == 0) {
            continue;
        }

        volatile struct ahci_spec_hba_port *const spec =
            (volatile struct ahci_spec_hba_port *)&regs->ports[index];

        if (!ahci_hba_probe_port(spec)) {
            printk(LOGLEVEL_WARN,
                   "ahci: implemented port #%" PRIu8 " is either not present "
                   "or inactive, or both\n",
                   index + 1);
            continue;
        }

        struct ahci_hba_port *const port = &device.port_list[usable_port_count];
        ahci_spec_hba_port_init(port, spec, index, &device);

        usable_port_count++;
    }

    if (usable_port_count == 0) {
        kfree(device.port_list);
        printk(LOGLEVEL_WARN,
               "ahci: no implemented ports are both present and active\n");

        return;
    }

    const uint32_t global_host_ctrl =
        mmio_read(&regs->global_host_control) |
        __AHCI_HBA_GLOBAL_HOST_CTRL_INT_ENABLE |
        __AHCI_HBA_GLOBAL_HOST_CTRL_AHCI_ENABLE;

    mmio_write(&regs->global_host_control, global_host_ctrl);
    printk(LOGLEVEL_INFO, "ahci: fully initialized\n");
}

static const struct pci_driver pci_driver = {
    .vendor = 0x1af4,
    .class = PCI_ENTITY_CLASS_MASS_STORAGE_CONTROLLER,
    .subclass = PCI_ENTITY_SUBCLASS_SATA,
    .prog_if = 0x1,
    .match =
        __PCI_DRIVER_MATCH_CLASS |
        __PCI_DRIVER_MATCH_SUBCLASS |
        __PCI_DRIVER_MATCH_PROGIF,
    .init = init_from_pci
};

__driver const struct driver driver = {
    .name = "x86_64-ahci-driver",
    .dtb = NULL,
    .pci = &pci_driver
};