/*
 * kernel/src/arch/x86_64/dev/ahci/port.c
 * © suhas pai
 */

#include "apic/lapic.h"

#include "asm/stack_trace.h"
#include "asm/pause.h"

#include "dev/printk.h"

#include "lib/bits.h"
#include "lib/size.h"

#include "mm/zone.h"
#include "sys/mmio.h"

#include "device.h"

#define AHCI_HBA_PORT_MAX_COUNT mib(4)
#define AHCI_HBA_CMD_TABLE_PAGE_ORDER 1

_Static_assert(
    ((uint64_t)sizeof(struct ahci_spec_hba_cmd_table) *
     // Each port (upto 32 exist) has a separate command-table
     sizeof_bits_field(struct ahci_spec_hba_registers, port_implemented))
        <= (PAGE_SIZE << AHCI_HBA_CMD_TABLE_PAGE_ORDER),
    "AHCI_HBA_CMD_TABLE_PAGE_ORDER is too low to fit all "
    "struct ahci_spec_hba_cmd_table entries");

__optimize(3)
static uint8_t find_free_command_header(struct ahci_hba_port *const port) {
    for (uint8_t index = 0;
         index != sizeof_bits_field(struct ahci_spec_hba_port, command_issue);
         index++)
    {
        if ((mmio_read(&port->spec->command_issue) & 1ull << index) == 0) {
            return index;
        }
    }

    return UINT8_MAX;
}

#define MAX_ATTEMPTS 100

__optimize(3) static
inline void flush_writes(volatile struct ahci_spec_hba_port *const port) {
    mmio_read(&port->cmd_status);
}

__optimize(3)
bool ahci_hba_port_start_running(struct ahci_hba_port *const port) {
    volatile struct ahci_spec_hba_port *const spec = port->spec;
    bool cmdlist_stopped = false;

    for (int i = 0; i != MAX_ATTEMPTS; i++) {
        if ((mmio_read(&spec->cmd_status) &
                __AHCI_HBA_PORT_CMDSTATUS_CMD_LIST_RUNNING) == 0)
        {
            cmdlist_stopped = true;
            break;
        }
    }

    if (!cmdlist_stopped) {
        return false;
    }

    mmio_write(&spec->cmd_status,
               mmio_read(&spec->cmd_status) |
                __AHCI_HBA_PORT_CMDSTATUS_FIS_RECEIVE_ENABLE);

    mmio_write(&spec->cmd_status,
               mmio_read(&spec->cmd_status) | __AHCI_HBA_PORT_CMDSTATUS_START);

    mmio_write(&spec->cmd_status,
               mmio_read(&spec->cmd_status) |
                __AHCI_HBA_PORT_CMDSTATUS_CMD_LIST_OVERRIDE);

    flush_writes(spec);
    return true;
}

__optimize(3)
bool ahci_hba_port_stop_running(struct ahci_hba_port *const port) {
    volatile struct ahci_spec_hba_port *const spec = port->spec;
    mmio_write(&spec->cmd_status,
               rm_mask(mmio_read(&spec->cmd_status),
                       __AHCI_HBA_PORT_CMDSTATUS_START));

    bool cmdlist_stopped = false;
    for (uint8_t i = 0; i != MAX_ATTEMPTS; i++) {
        if ((mmio_read(&spec->cmd_status) &
                __AHCI_HBA_PORT_CMDSTATUS_CMD_LIST_RUNNING) == 0)
        {
            cmdlist_stopped = true;
            break;
        }

        cpu_pause();
    }

    if (!cmdlist_stopped) {
        return false;
    }

    mmio_write(&spec->cmd_status,
               rm_mask(mmio_read(&spec->cmd_status),
                       __AHCI_HBA_PORT_CMDSTATUS_FIS_RECEIVE_ENABLE));

    bool fis_stopped = false;
    for (uint8_t i = 0; i != MAX_ATTEMPTS; i++) {
        if ((mmio_read(&spec->cmd_status) &
                __AHCI_HBA_PORT_CMDSTATUS_FIS_RECEIVE_RUNNING) == 0)
        {
            fis_stopped = true;
            break;
        }

        cpu_pause();
    }

    return fis_stopped;
}

__optimize(3) static
inline void clear_error_bits(volatile struct ahci_spec_hba_port *const port) {
    mmio_write(&port->interrupt_status, mmio_read(&port->interrupt_status));
}

__optimize(3) static inline
bool wait_for_tfd_idle(volatile struct ahci_spec_hba_port *const port) {
    const uint32_t tfd_flags =
        __AHCI_HBA_TFD_STATUS_BUSY |
        __AHCI_HBA_TFD_STATUS_DATA_TRANSFER_REQUESTED;

    for (uint8_t i = 0; i != MAX_ATTEMPTS; i++) {
        if ((mmio_read(&port->task_file_data) & tfd_flags) == 0) {
            return true;
        }

        cpu_pause();
    }

    return false;
}

__optimize(3) static bool comreset_port(struct ahci_hba_port *const port) {
    // Spec v1.3.1, §10.4.2 Port Reset

    volatile struct ahci_spec_hba_port *const spec = port->spec;
    const uint32_t new_sata_control =
        __AHCI_HBA_PORT_SATA_STAT_CTRL_IPM <<
            AHCI_HBA_PORT_SATA_STAT_CTRL_IPM_SHIFT |
        AHCI_HBA_PORT_DET_INIT;

    mmio_write(&spec->sata_control, new_sata_control);
    flush_writes(spec);

    mmio_write(&spec->sata_control,
               rm_mask(mmio_read(&spec->sata_control),
                       __AHCI_HBA_PORT_SATA_STAT_CTRL_DET) |
               AHCI_HBA_PORT_DET_NO_INIT);

    flush_writes(spec);
    return true;
}

__optimize(3) static inline
bool wait_until_det_present(volatile struct ahci_spec_hba_port *const port) {
    for (uint8_t i = 0; i != MAX_ATTEMPTS; i++) {
        if ((mmio_read(&port->sata_status) & AHCI_HBA_PORT_DET_PRESENT) == 0) {
            return true;
        }

        cpu_pause();
    }

    return false;
}

__optimize(3)
__unused static bool reset_port(struct ahci_hba_port *const port) {
    if (!ahci_hba_port_stop(port)) {
        return false;
    }

    volatile struct ahci_spec_hba_port *const spec = port->spec;

    clear_error_bits(spec);
    if (!wait_for_tfd_idle(spec)) {
        comreset_port(port);
        return false;
    }

    ahci_hba_port_start(port);
    return wait_until_det_present(spec);
}

__optimize(3) static void print_serr_error(const uint32_t serr) {
    enum ahci_hba_port_sata_error_flags err_flag =
        __AHCI_HBA_PORT_SATA_ERROR_SERR_DATA_INTEGRITY;

    switch (err_flag) {
    #define ERROR_CASE(flag, msg, ...) \
        case flag: \
            do { \
                if (serr & flag) { \
                    printk(LOGLEVEL_WARN, (msg), ##__VA_ARGS__); \
                } \
            } while (false)

        ERROR_CASE(__AHCI_HBA_PORT_SATA_ERROR_SERR_DATA_INTEGRITY,
                   "ahci: encountered data-integrity issue\n\n");
            __attribute__((fallthrough));
        ERROR_CASE(__AHCI_HBA_PORT_SATA_ERROR_SERR_COMM,
                   "ahci: encountered communication issue\n");
            __attribute__((fallthrough));
        ERROR_CASE(__AHCI_HBA_PORT_SATA_ERROR_SERR_TRANSIENT_DATA_INTEGRITY,
                   "ahci: encountered transient data-integrity issue\n\n");
            __attribute__((fallthrough));
        ERROR_CASE(
            __AHCI_HBA_PORT_SATA_ERROR_SERR_PERSISTENT_COMM_OR_INTEGRITY,
            "ahci: encountered persistent communication or data-integrity "
            "issue\n");
            __attribute__((fallthrough));
        ERROR_CASE(__AHCI_HBA_PORT_SATA_ERROR_SATA_PROTOCOL,
                   "ahci: encountered sata-protocol error\n");
            __attribute__((fallthrough));
        ERROR_CASE(__AHCI_HBA_PORT_SATA_ERROR_SATA_INTERNAL_ERROR,
                   "ahci: encountered internal sata error\n");
            break;

    #undef ERROR_CASE
    }

    printk(LOGLEVEL_WARN, "ahci: serr has an unrecognized error\n");
}

__optimize(3) static void print_serr_diag(const uint32_t serr) {
    enum ahci_hba_port_sata_diag_flags diag_flag =
        __AHCI_HBA_PORT_SATA_DIAG_PHYRDY_CHANGED;

    switch (diag_flag) {
    #define DIAG_CASE(name, msg, ...) \
        case name: \
            do { \
                if (serr & name) { \
                    printk(LOGLEVEL_WARN, msg, ##__VA_ARGS__); \
                } \
            } while (false)

        DIAG_CASE(__AHCI_HBA_PORT_SATA_DIAG_PHYRDY_CHANGED,
                  "ahci: diagnostic: phyrdy changed\n");
            __attribute__((fallthrough));
        DIAG_CASE(__AHCI_HBA_PORT_SATA_DIAG_PHY_INTERNAL_ERROR,
                  "ahci: diagnostic: phy internal changed\n");
            __attribute__((fallthrough));
        DIAG_CASE(__AHCI_HBA_PORT_SATA_DIAG_COMM_WAKE,
                  "ahci: diagnostic: communication wake\n");
            __attribute__((fallthrough));
        DIAG_CASE(__AHCI_HBA_PORT_SATA_DIAG_10B_TO_8B_DECODE_ERROR,
                  "ahci: diagnostic: 18b to 8b decode error\n");
            __attribute__((fallthrough));
        DIAG_CASE(__AHCI_HBA_PORT_SATA_DIAG_DISPARITY_ERROR,
                  "ahci: diagnostic: disparity error\n");
            __attribute__((fallthrough));
        DIAG_CASE(__AHCI_HBA_PORT_SATA_DIAG_HANDSHAKE_ERROR,
                  "ahci: diagnostic: handshake error\n");
            __attribute__((fallthrough));
        DIAG_CASE(__AHCI_HBA_PORT_SATA_DIAG_LINK_SEQ_ERROR,
                  "ahci: diagnostic: link sequence error\n");
            __attribute__((fallthrough));
        DIAG_CASE(
            __AHCI_HBA_PORT_SATA_DIAG_TRANSPORT_STATE_TRANSITION_ERROR,
            "ahci: diagnostic: transport state transition error\n");
            __attribute__((fallthrough));
        DIAG_CASE(__AHCI_HBA_PORT_SATA_DIAG_UNKNOWN_FIS_TYPE,
                  "ahci: diagnostic: unknown fis type\n");
            __attribute__((fallthrough));
        DIAG_CASE(__AHCI_HBA_PORT_SATA_DIAG_EXCHANGED,
                  "ahci: diagnostic: exchanged\n");
            break;
    #undef DIAG_CASE
    }

    printk(LOGLEVEL_WARN, "ahci: serr has an unrecognized diag\n");
}

__optimize(3) static void handle_serr(struct ahci_hba_port *const port) {
    const uint32_t serr = mmio_read(&port->spec->sata_error);

    print_serr_error(serr);
    print_serr_diag(serr);
}

__optimize(3)
void ahci_port_handle_irq(const uint64_t vector, irq_context_t *const frame) {
    (void)vector;
    (void)frame;

    struct ahci_hba_device *const hba = ahci_hba_get();
    const uint32_t pending_ports = mmio_read(&hba->regs->interrupt_status);

    // Write to interrupt-status to clear bits
    mmio_write(&hba->regs->interrupt_status, pending_ports);
    const uint32_t fatal_mask =
        __AHCI_HBA_IS_HOST_BUS_FATAL_ERR_STATUS |
        __AHCI_HBA_IS_HOST_BUS_DATA_ERR_STATUS |
        __AHCI_HBA_IS_INTERFACE_FATAL_ERR_STATUS |
        __AHCI_HBA_IS_TASK_FILE_ERR_STATUS;

    for_every_lsb_one_bit(pending_ports, /*start_index=*/0, pending_index) {
        for (uint32_t index = 0; index != hba->port_count; index++) {
            struct ahci_hba_port *const port = &hba->port_list[index];
            if (port->index != pending_index) {
                continue;
            }

            mmio_write(&port->spec->interrupt_enable,
                       __AHCI_HBA_IE_DEV_TO_HOST_FIS |
                       __AHCI_HBA_IE_UNKNOWN_FIS |
                       __AHCI_HBA_IE_PORT_CHANGE |
                       __AHCI_HBA_IE_DEV_MECH_PRESENCE |
                       __AHCI_HBA_IE_PHYRDY_CHANGE_STATUS |
                       __AHCI_HBA_IE_INCORRECT_PORT_MULT_STATUS |
                       __AHCI_HBA_IE_OVERFLOW_STATUS |
                       __AHCI_HBA_IE_INTERFACE_NOT_FATAL_ERR_STATUS |
                       __AHCI_HBA_IE_INTERFACE_FATAL_ERR_STATUS |
                       __AHCI_HBA_IE_HOST_BUS_DATA_ERR_STATUS |
                       __AHCI_HBA_IE_HOST_BUS_FATAL_ERR_STATUS |
                       __AHCI_HBA_IE_TASK_FILE_ERR_STATUS);

            const uint32_t interrupt_status =
                mmio_read(&port->spec->interrupt_status);

            // Write to interrupt-status to clear bits

            mmio_write(&port->spec->interrupt_status, interrupt_status);
            event_trigger(&port->event, /*drop_if_no_listeners=*/false);

            if (interrupt_status & fatal_mask) {
                handle_serr(port);
                print_stack_trace_from_top(
                    (struct stack_trace *)frame->regs.rbp,
                    /*max_lines=*/10);
            }

            break;
        }
    }

    lapic_eoi();
}

__optimize(3) bool ahci_hba_port_start(struct ahci_hba_port *const port) {
    ahci_hba_port_start_running(port);

    volatile struct ahci_spec_hba_port *const spec = port->spec;
    mmio_write(&spec->interrupt_enable,
               __AHCI_HBA_IE_DEV_TO_HOST_FIS |
               __AHCI_HBA_IE_UNKNOWN_FIS |
               __AHCI_HBA_IE_PORT_CHANGE |
               __AHCI_HBA_IE_DEV_MECH_PRESENCE |
               __AHCI_HBA_IE_PHYRDY_CHANGE_STATUS |
               __AHCI_HBA_IE_INCORRECT_PORT_MULT_STATUS |
               __AHCI_HBA_IE_OVERFLOW_STATUS |
               __AHCI_HBA_IE_INTERFACE_NOT_FATAL_ERR_STATUS |
               __AHCI_HBA_IE_INTERFACE_FATAL_ERR_STATUS |
               __AHCI_HBA_IE_HOST_BUS_DATA_ERR_STATUS |
               __AHCI_HBA_IE_HOST_BUS_FATAL_ERR_STATUS |
               __AHCI_HBA_IE_TASK_FILE_ERR_STATUS);

    flush_writes(spec);
    return true;
}

__optimize(3) static void
ahci_hba_port_power_on_and_spin_up(
    volatile struct ahci_spec_hba_port *const port,
    struct ahci_hba_device *const device,
    const uint8_t index)
{
    const uint32_t cmd_status = mmio_read(&port->cmd_status);
    if (cmd_status & __AHCI_HBA_PORT_CMDSTATUS_COLD_PRESENCE_DETECTION) {
        mmio_write(&port->cmd_status,
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
        mmio_write(&port->cmd_status,
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
    uint32_t cmd_status = mmio_read(&port->cmd_status);
    cmd_status =
        rm_mask(cmd_status, __AHCI_HBA_PORT_CMDSTATUS_INTERFACE_COMM_CTRL) |
        ctrl << AHCI_HBA_PORT_CMDSTATUS_INTERFACE_COMM_CTRL_SHIFT;

    mmio_write(&port->cmd_status, cmd_status);
}

__optimize(3) void ahci_spec_hba_port_init(struct ahci_hba_port *const port) {
    if (!ahci_hba_port_stop_running(port)) {
        printk(LOGLEVEL_WARN,
               "ahci: failed to stop port #%" PRIu8 " before init\n",
               port->index + 1);
        return;
    }

    port->sig = (enum sata_sig)mmio_read(&port->spec->signature);
    switch (port->sig) {
        case SATA_SIG_ATA:
            printk(LOGLEVEL_INFO,
                   "ahci: port #%" PRIu8 " is a sata device\n",
                   port->index + 1);
            break;
        case SATA_SIG_ATAPI:
            printk(LOGLEVEL_INFO,
                   "ahci: port #%" PRIu8 " is a satapi device\n",
                   port->index + 1);
            break;
        case SATA_SIG_SEMB:
            printk(LOGLEVEL_INFO,
                   "ahci: port #%" PRIu8 " is a semb device. ignoring\n",
                   port->index + 1);
            return;
        case SATA_SIG_PM:
            printk(LOGLEVEL_INFO,
                   "ahci: port #%" PRIu8 " is a pm device. ignoring\n",
                   port->index + 1);
            return;
    }

    struct page *cmd_list_page = NULL;
    struct page *cmd_table_pages = NULL;

    const bool supports_64bit_dma = port->device->supports_64bit_dma;
    if (supports_64bit_dma) {
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
    volatile struct ahci_spec_hba_port *const spec = port->spec;

    mmio_write(&spec->cmd_list_base_phys_lower32, cmd_list_phys);
    if (supports_64bit_dma) {
        mmio_write(&spec->cmd_list_base_phys_upper32, cmd_list_phys >> 32);
    }

    printk(LOGLEVEL_INFO,
           "ahci: port #%" PRIu8 " has a cmd-list base at %p\n",
           port->index + 1,
           (void *)phys_range.front);

    volatile struct ahci_spec_port_cmdhdr *cmd_header =
        phys_to_virt(cmd_list_phys);
    const volatile struct ahci_spec_port_cmdhdr *const end =
        cmd_header + AHCI_HBA_CMD_HDR_COUNT;

    for (uint64_t cmdtable_phys = phys_range.front;
         cmd_header != end;
         cmd_header++,
         cmdtable_phys += sizeof(struct ahci_spec_hba_cmd_table))
    {
        mmio_write(&cmd_header->cmd_table_base_lower32,
                   (uint32_t)cmdtable_phys);

        if (supports_64bit_dma) {
            mmio_write(&cmd_header->cmd_table_base_upper32,
                       cmdtable_phys >> 32);
        }
    }

    struct mmio_region *const mmio =
        vmap_mmio(phys_range, PROT_READ | PROT_WRITE, /*flags=*/0);

    if (mmio == NULL) {
        free_page(cmd_list_page);
        free_pages(cmd_table_pages, AHCI_HBA_CMD_TABLE_PAGE_ORDER);

        return;
    }

    port->headers = phys_to_virt(cmd_list_phys);
    port->cmdtable_phys = page_to_phys(cmd_table_pages);
    port->event = EVENT_INIT();
    port->mmio = mmio;

    struct page *const resp_page =
        alloc_page(PAGE_STATE_USED, /*alloc_flags=*/0);

    if (resp_page == NULL) {
        free_page(cmd_list_page);
        free_pages(cmd_table_pages, AHCI_HBA_CMD_TABLE_PAGE_ORDER);

        return;
    }

    clear_error_bits(spec);

    ahci_hba_port_power_on_and_spin_up(spec, port->device, port->index);
    ahci_hba_port_set_state(spec, AHCI_HBA_PORT_INTERFACE_COMM_CTRL_ACTIVE);

    mmio_write(&spec->cmd_status,
               mmio_read(&spec->cmd_status) |
                __AHCI_HBA_PORT_CMDSTATUS_FIS_RECEIVE_ENABLE);

    flush_writes(spec);
    if (!ahci_hba_port_start(port)) {
        return;
    }

    struct await_result identify_result = AWAIT_RESULT_NULL();
    const bool result =
        ahci_hba_port_send_ata_command(port,
                                       port->sig == SATA_SIG_ATAPI ?
                                        ATA_CMD_IDENTIFY_PACKET :
                                        ATA_CMD_IDENTIFY,
                                       /*sector=*/0,
                                       /*count=*/PAGE_SIZE,
                                       page_to_phys(resp_page),
                                       &identify_result);

    if (!result) {
        return;
    }

    events_await(&identify_result.event, /*event_count=*/1, /*block=*/true);
    const struct atapi_identify *const ident =
        (const struct atapi_identify *)identify_result.result_ptr;

    printk(LOGLEVEL_INFO,
           "ahci: identify response:\n"
           "\tdevice-type: 0x%" PRIx16 "\n"
           "\tserial: " SV_FMT "\n"
           "\tmodel: " SV_FMT "\n"
           "\tcapabilities: 0x%" PRIx32 "\n"
           "\t\tdma supported: %s\n"
           "\t\tlba supported: %s\n",
           ident->device_type,
           SV_FMT_ARGS(sv_of_carr(ident->serial)),
           SV_FMT_ARGS(sv_of_carr(ident->model)),
           ident->capabilities,
           ident->capabilities & __ATA_IDENTITY_CAP_DMA_SUPPORT ? "yes" : "no",
           ident->capabilities & __ATA_IDENTITY_CAP_LBA_SUPPORT ? "yes" : "no");

    printk(LOGLEVEL_INFO,
           "ahci: port #%" PRIu8 " initialized\n",
           port->index + 1);

    free_page(resp_page);
}

__optimize(3) bool
ahci_hba_port_send_command(struct ahci_hba_port *const port,
                           const enum ahci_hba_port_command_kind command_kind,
                           const uint64_t sector,
                           const uint16_t count,
                           const uint64_t response_phys,
                           struct await_result *const result_out)
{
    enum ata_command command = ATA_CMD_READ_DMA;
    switch (command_kind) {
        case AHCI_HBA_PORT_CMDKIND_READ:
            command =
                sector >= 1ull << 48 ? ATA_CMD_READ_DMA_EXT : ATA_CMD_READ_DMA;
            break;
        case AHCI_HBA_PORT_CMDKIND_WRITE:
            command =
                sector >= 1ull << 48 ?
                    ATA_CMD_WRITE_DMA_EXT : ATA_CMD_WRITE_DMA;
            break;
    }

    return ahci_hba_port_send_ata_command(port,
                                          command,
                                          sector,
                                          count,
                                          response_phys,
                                          result_out);
}

bool
ahci_hba_port_send_ata_command(struct ahci_hba_port *const port,
                               const enum ata_command command_kind,
                               const uint64_t sector,
                               const uint16_t count,
                               const uint64_t phys_addr,
                               struct await_result *const result_out)
{
    if (!ahci_hba_port_stop_running(port)) {
        printk(LOGLEVEL_WARN,
               "ahci: failed to stop port #%" PRIu8 "\n",
               port->index + 1);
        return false;
    }

    if (__builtin_expect(count == 0, 0)) {
        printk(LOGLEVEL_WARN,
               "ahci-port: port #%" PRIu8 " got a h2d request of 0 bytes\n",
               port->index + 1);
        return true;
    }

    const uint8_t slot = find_free_command_header(port);
    if (slot == UINT8_MAX) {
        printk(LOGLEVEL_WARN,
               "ahci-port: port #%" PRIu8 " has no free command-headers\n",
               port->index + 1);
        return false;
    }

    // Clear any pending interrupts
    mmio_write(&port->spec->interrupt_status, 0);
    volatile struct ahci_spec_port_cmdhdr *const cmd_header =
        &port->headers[slot];

    uint16_t flags = sizeof(struct ahci_spec_fis_reg_h2d) / sizeof(uint32_t);
    switch (command_kind) {
        case ATA_CMD_READ_PIO:
        case ATA_CMD_READ_PIO_EXT:
        case ATA_CMD_READ_DMA:
        case ATA_CMD_READ_DMA_EXT:
        case ATA_CMD_IDENTIFY_PACKET:
        case ATA_CMD_IDENTIFY:
        case ATA_CMD_CACHE_FLUSH:
        case ATA_CMD_CACHE_FLUSH_EXT:
            flags = rm_mask(flags, AHCI_HBA_PORT_CMDKIND_WRITE);
            break;
        case ATA_CMD_PACKET:
            verify_not_reached();
            break;
        case ATA_CMD_WRITE_PIO:
        case ATA_CMD_WRITE_PIO_EXT:
        case ATA_CMD_WRITE_DMA:
        case ATA_CMD_WRITE_DMA_EXT:
            flags |= AHCI_HBA_PORT_CMDKIND_WRITE;
            break;
    }

    const uint32_t prdt_count = div_round_up(count, AHCI_HBA_PORT_MAX_COUNT);

    mmio_write(&cmd_header->flags, flags);
    mmio_write(&cmd_header->prdt_length, prdt_count);

    struct ahci_spec_hba_cmd_table *const cmd_table =
        phys_to_virt(port->cmdtable_phys);
    volatile struct ahci_spec_hba_prdt_entry *const entries =
        cmd_table->prdt_entries;

    uint64_t response_addr = phys_addr;
    const uint32_t entry_flags =
        (AHCI_HBA_PORT_MAX_COUNT - 1) |
        __AHCI_SPEC_HBA_PRDT_ENTRY_INT_ON_COMPLETION;

    for (uint8_t i = 0;
         i < prdt_count - 1;
         i++, response_addr += AHCI_HBA_PORT_MAX_COUNT)
    {
        volatile struct ahci_spec_hba_prdt_entry *const entry = &entries[i];

        mmio_write(&entry->data_base_address_lower32, (uint32_t)response_addr);
        if (port->device->supports_64bit_dma) {
            mmio_write(&entry->data_base_address_upper32, response_addr >> 32);
        }

        mmio_write(&entry->flags, entry_flags);
    }

    volatile struct ahci_spec_hba_prdt_entry *const last_entry =
        &entries[prdt_count - 1];

    const uint32_t last_entry_byte_count = count % AHCI_HBA_PORT_MAX_COUNT;
    uint32_t last_entry_flags = __AHCI_SPEC_HBA_PRDT_ENTRY_INT_ON_COMPLETION;

    if (last_entry_byte_count != 0) {
        last_entry_flags |= last_entry_byte_count - 1;
    } else {
        last_entry_flags |= AHCI_HBA_PORT_MAX_COUNT - 1;
    }

    mmio_write(&last_entry->data_base_address_lower32, (uint32_t)response_addr);
    if (port->device->supports_64bit_dma) {
        mmio_write(&last_entry->data_base_address_upper32, response_addr >> 32);
    }

    mmio_write(&last_entry->flags, last_entry_flags);

    struct ahci_spec_fis_reg_h2d *const h2d_fis =
        (struct ahci_spec_fis_reg_h2d *)(uint64_t)cmd_table->command_fis;

    h2d_fis->fis_type = AHCI_FIS_KIND_REG_H2D;
    h2d_fis->flags = __AHCI_FIS_REG_H2D_IS_ATA_CMD;

    h2d_fis->command = command_kind;
    h2d_fis->feature_low8 = 0;

    h2d_fis->lba0 = (uint8_t)sector;
    h2d_fis->lba1 = (uint8_t)(sector >> 8);
    h2d_fis->lba2 = (uint8_t)(sector >> 16);

    h2d_fis->device = __ATA_USE_LBA_ADDRESSING;

    h2d_fis->lba3 = (uint8_t)(sector >> 24);
    h2d_fis->lba4 = (uint8_t)(sector >> 32);
    h2d_fis->lba5 = (uint8_t)(sector >> 40);

    h2d_fis->feature_high8 = 0;
    h2d_fis->count_low = (uint8_t)count;
    h2d_fis->count_high = (uint8_t)(count >> 8);
    h2d_fis->icc = 0;
    h2d_fis->control = 0;

    if (!wait_for_tfd_idle(port->spec)) {
        printk(LOGLEVEL_WARN,
               "ahci: failed to wait tfd for port #%" PRIu8 "\n",
               port->index + 1);
        return false;
    }

    if (!ahci_hba_port_start_running(port)) {
        printk(LOGLEVEL_WARN,
               "ahci: failed to restart port #%" PRIu8 "\n",
               port->index + 1);
        return false;
    }

    result_out->event = &port->event;
    result_out->result_ptr = phys_to_virt(phys_addr);

    mmio_write(&port->spec->command_issue, 1ull << slot);
    return true;
}
