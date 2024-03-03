/*
 * kernel/src/arch/x86_64/dev/ahci/port.c
 * © suhas pai
 */

#include "dev/ata/atapi.h"
#include "dev/ata/defines.h"

#include "dev/scsi/swap.h"

#include "apic/lapic.h"
#include "asm/pause.h"

#include "dev/printk.h"

#include "lib/bits.h"
#include "lib/size.h"

#include "mm/kmalloc.h"
#include "mm/zone.h"

#include "sys/mmio.h"
#include "device.h"

#define AHCI_HBA_PORT_MAX_COUNT mib(4)
#define AHCI_HBA_CMD_TABLE_PAGE_ORDER 1

_Static_assert(
    ((uint64_t)sizeof(struct ahci_spec_hba_cmd_table) *
     // Each port (upto 32 exist) has a separate command-table
     AHCI_HBA_MAX_PORT_COUNT) <= (PAGE_SIZE << AHCI_HBA_CMD_TABLE_PAGE_ORDER),
    "AHCI_HBA_CMD_TABLE_PAGE_ORDER is too low to fit all "
    "struct ahci_spec_hba_cmd_table entries");

__optimize(3)
static uint8_t find_free_cmdhdr(struct ahci_hba_port *const port) {
    const int flag = spin_acquire_with_irq(&port->lock);
    if (port->ports_bitset == UINT32_MAX) {
        spin_release_with_irq(&port->lock, flag);
        return UINT8_MAX;
    }

    const uint8_t slot =
       find_lsb_zero_bit(port->ports_bitset, /*start_index=*/0);

    if (slot > sizeof_bits(uint32_t)) {
        spin_release_with_irq(&port->lock, flag);
        return UINT8_MAX;
    }

    port->ports_bitset |= 1ul << slot;
    spin_release_with_irq(&port->lock, flag);

    return slot;
}

#define MAX_ATTEMPTS 100

__optimize(3) static inline
void flush_writes(volatile struct ahci_spec_hba_port *const port) {
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
        printk(LOGLEVEL_WARN,
               "ahci: failed to stop port #%" PRIu8 " to restart\n",
               port->index + 1);

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

__optimize(3)
static inline bool wait_for_tfd_idle(struct ahci_hba_port *const port) {
    const uint32_t tfd_flags =
        __AHCI_HBA_TFD_STATUS_BUSY |
        __AHCI_HBA_TFD_STATUS_DATA_TRANSFER_REQUESTED;

    volatile struct ahci_spec_hba_port *const spec = port->spec;
    for (uint8_t i = 0; i != MAX_ATTEMPTS; i++) {
        if ((mmio_read(&spec->task_file_data) & tfd_flags) == 0) {
            return true;
        }

        cpu_pause();
    }

    printk(LOGLEVEL_WARN,
           "ahci: failed to wait for tfd busy flag to clear for port "
           "#%" PRIu8 "\n",
           port->index + 1);

    return false;
}

__optimize(3) static void comreset_port(struct ahci_hba_port *const port) {
    // Spec v1.3.1, §10.4.2 Port Reset
    volatile struct ahci_spec_hba_port *const spec = port->spec;
    {
        const uint32_t new_sata_control =
             AHCI_HBA_PORT_DET_INIT |
            __AHCI_HBA_PORT_SATA_STAT_CTRL_IPM <<
                AHCI_HBA_PORT_SATA_STAT_CTRL_IPM_SHIFT;

        mmio_write(&spec->sata_control, new_sata_control);
        flush_writes(spec);
    }
    {
        const uint32_t new_sata_control =
            rm_mask(mmio_read(&spec->sata_control),
                    __AHCI_HBA_PORT_SATA_STAT_CTRL_DET) |
            AHCI_HBA_PORT_DET_NO_INIT;

        mmio_write(&spec->sata_control, new_sata_control);
        flush_writes(spec);
    }
}

__optimize(3)
static inline bool wait_until_det_present(struct ahci_hba_port *const port) {
    volatile struct ahci_spec_hba_port *const spec = port->spec;
    for (uint8_t i = 0; i != MAX_ATTEMPTS; i++) {
        if ((mmio_read(&spec->sata_status) & AHCI_HBA_PORT_DET_PRESENT) == 0) {
            return true;
        }

        cpu_pause();
    }

    printk(LOGLEVEL_WARN,
           "ahci: det flag for port #%" PRIu8 " is set\n",
           port->index + 1);

    return false;
}

__optimize(3) static bool reset_port(struct ahci_hba_port *const port) {
    if (!ahci_hba_port_stop(port)) {
        printk(LOGLEVEL_WARN,
               "ahci: failed to reset port #%" PRIu8 ".\n",
               port->index + 1);

        return false;
    }

    volatile struct ahci_spec_hba_port *const spec = port->spec;

    clear_error_bits(spec);
    if (!wait_for_tfd_idle(port)) {
        printk(LOGLEVEL_WARN,
               "ahci: failed to reset port #%" PRIu8 ". issuing comreset\n",
               port->index + 1);

        comreset_port(port);
    }

    if (!ahci_hba_port_start(port) || !wait_until_det_present(port)) {
        printk(LOGLEVEL_WARN,
               "ahci: failed to reset port #%" PRIu8 "\n",
               port->index + 1);
        return false;
    }

    return true;
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
            [[fallthrough]];
        ERROR_CASE(__AHCI_HBA_PORT_SATA_ERROR_SERR_COMM,
                   "ahci: encountered communication issue\n");
            [[fallthrough]];
        ERROR_CASE(__AHCI_HBA_PORT_SATA_ERROR_SERR_TRANSIENT_DATA_INTEGRITY,
                   "ahci: encountered transient data-integrity issue\n\n");
            [[fallthrough]];
        ERROR_CASE(
            __AHCI_HBA_PORT_SATA_ERROR_SERR_PERSISTENT_COMM_OR_INTEGRITY,
            "ahci: encountered persistent communication or data-integrity "
            "issue\n");
            [[fallthrough]];
        ERROR_CASE(__AHCI_HBA_PORT_SATA_ERROR_SATA_PROTOCOL,
                   "ahci: encountered sata-protocol error\n");
            [[fallthrough]];
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
            [[fallthrough]];
        DIAG_CASE(__AHCI_HBA_PORT_SATA_DIAG_PHY_INTERNAL_ERROR,
                  "ahci: diagnostic: phy internal changed\n");
            [[fallthrough]];
        DIAG_CASE(__AHCI_HBA_PORT_SATA_DIAG_COMM_WAKE,
                  "ahci: diagnostic: communication wake\n");
            [[fallthrough]];
        DIAG_CASE(__AHCI_HBA_PORT_SATA_DIAG_10B_TO_8B_DECODE_ERROR,
                  "ahci: diagnostic: 18b to 8b decode error\n");
            [[fallthrough]];
        DIAG_CASE(__AHCI_HBA_PORT_SATA_DIAG_DISPARITY_ERROR,
                  "ahci: diagnostic: disparity error\n");
            [[fallthrough]];
        DIAG_CASE(__AHCI_HBA_PORT_SATA_DIAG_HANDSHAKE_ERROR,
                  "ahci: diagnostic: handshake error\n");
            [[fallthrough]];
        DIAG_CASE(__AHCI_HBA_PORT_SATA_DIAG_LINK_SEQ_ERROR,
                  "ahci: diagnostic: link sequence error\n");
            [[fallthrough]];
        DIAG_CASE(
            __AHCI_HBA_PORT_SATA_DIAG_TRANSPORT_STATE_TRANSITION_ERROR,
            "ahci: diagnostic: transport state transition error\n");
            [[fallthrough]];
        DIAG_CASE(__AHCI_HBA_PORT_SATA_DIAG_UNKNOWN_FIS_TYPE,
                  "ahci: diagnostic: unknown fis type\n");
            [[fallthrough]];
        DIAG_CASE(__AHCI_HBA_PORT_SATA_DIAG_EXCHANGED,
                  "ahci: diagnostic: exchanged\n");
            break;
    #undef DIAG_CASE
    }

    printk(LOGLEVEL_WARN, "ahci: serr has an unrecognized diag\n");
}

__optimize(3) static void
handle_error(struct ahci_hba_port *const port, const uint32_t interrupt_status)
{
    if (interrupt_status & AHCI_HBA_PORT_IS_RESET_REQ_FLAGS) {
        port->state = AHCI_HBA_PORT_STATE_NEEDS_RESET;
    }
}

__optimize(3) static
void enable_port_interrupts(volatile struct ahci_spec_hba_port *const port) {
    mmio_write(&port->interrupt_enable,
               __AHCI_HBA_IE_DEV_TO_HOST_FIS |
               __AHCI_HBA_IE_PORT_CHANGE |
               AHCI_HBA_PORT_IE_ERROR_FLAGS);
}

__optimize(3) void
ahci_port_handle_irq(const uint64_t vector,
                     struct thread_context *const context)
{
    (void)vector;
    (void)context;

    struct ahci_hba_port *ports_with_results[AHCI_HBA_MAX_PORT_COUNT] = {0};
    uint8_t port_count = 0;

    uint32_t port_interrupt_status[AHCI_HBA_MAX_PORT_COUNT] = {0};
    uint32_t finished_cmdhdrs[AHCI_HBA_CMD_HDR_COUNT] = {0};

    struct ahci_hba_device *const hba = ahci_hba_get();
    const uint32_t pending_ports = mmio_read(&hba->regs->interrupt_status);

    if (pending_ports == 0) {
        printk(LOGLEVEL_INFO, "ahci: got spurious interrupt\n");
        lapic_eoi();

        return;
    }

    // Write to interrupt-status to clear bits
    mmio_write(&hba->regs->interrupt_status, pending_ports);
    for_each_lsb_one_bit(pending_ports, /*start_index=*/0, pending_index) {
        for (uint32_t index = 0; index != hba->port_count; index++) {
            struct ahci_hba_port *const port = &hba->port_list[index];
            if (port->index != pending_index) {
                continue;
            }

            volatile struct ahci_spec_hba_port *const spec = port->spec;

            uint32_t ci = mmio_read(&spec->command_issue);
            const uint32_t interrupt_status =
                mmio_read(&spec->interrupt_status);

            // Write to interrupt-status to clear bits.
            mmio_write(&spec->interrupt_status, interrupt_status);
            if (interrupt_status & AHCI_HBA_PORT_IS_ERROR_FLAGS) {
                port->error.serr = mmio_read(&spec->sata_error);
                port->error.interrupt_status = interrupt_status;

                handle_error(port, interrupt_status);
                ci = ~port->ports_bitset;
            }

            spin_acquire(&port->lock);
            finished_cmdhdrs[port_count] = port->ports_bitset & ~ci;
            port->ports_bitset =
                rm_mask(port->ports_bitset, finished_cmdhdrs[port_count]);
            spin_release(&port->lock);

            ports_with_results[port_count] = port;
            port_interrupt_status[port_count] = interrupt_status;

            port_count++;
            break;
        }
    }

    lapic_eoi();
    for (uint8_t i = 0; i != port_count; i++) {
        struct ahci_hba_port *const port = ports_with_results[i];

        const uint32_t interrupt_status = port_interrupt_status[i];
        const bool result =
            (interrupt_status & AHCI_HBA_PORT_IS_ERROR_FLAGS) == 0;

        const struct await_result await_result = AWAIT_RESULT_BOOL(result);
        for_each_lsb_one_bit(finished_cmdhdrs[i], /*start_index=*/0, iter) {
            struct event *const event = &port->cmdhdr_info_list[iter].event;
            event_trigger(event, &await_result, /*drop_if_no_listeners=*/false);
        }

        enable_port_interrupts(port->spec);
    }
}

__optimize(3) bool ahci_hba_port_start(struct ahci_hba_port *const port) {
    if (!ahci_hba_port_start_running(port)) {
        return false;
    }

    enable_port_interrupts(port->spec);
    flush_writes(port->spec);

    return true;
}

__optimize(3) bool ahci_hba_port_stop(struct ahci_hba_port *const port) {
    volatile struct ahci_spec_hba_port *const spec = port->spec;
    const uint32_t cmd_status = mmio_read(&spec->cmd_status);

    if (cmd_status & __AHCI_HBA_PORT_CMDSTATUS_START) {
        mmio_write(&spec->cmd_status,
                   rm_mask(cmd_status, __AHCI_HBA_PORT_CMDSTATUS_START));
    }

    for (int i = 0; i != MAX_ATTEMPTS; i++) {
        if ((mmio_read(&spec->cmd_status) &
                __AHCI_HBA_PORT_CMDSTATUS_CMD_LIST_RUNNING) == 0)
        {
            return true;
        }
    }

    printk(LOGLEVEL_WARN,
           "ahci: failed to stop port #%" PRIu8 "\n",
           port->index + 1);

    return false;
}

__optimize(3) static void
ahci_hba_port_power_on_and_spin_up(
    volatile struct ahci_spec_hba_port *const port,
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

    if (ahci_hba_get()->supports_staggered_spinup) {
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

__optimize(3)
static inline bool recognize_port_sig(struct ahci_hba_port *const port) {
    switch (port->sig) {
        case SATA_SIG_ATA:
            printk(LOGLEVEL_INFO,
                   "ahci: port #%" PRIu8 " is a sata device\n",
                   port->index + 1);
            return true;
        case SATA_SIG_ATAPI:
            printk(LOGLEVEL_INFO,
                   "ahci: port #%" PRIu8 " is a satapi device\n",
                   port->index + 1);
            return true;
        case SATA_SIG_SEMB:
            printk(LOGLEVEL_INFO,
                   "ahci: port #%" PRIu8 " is a semb device. ignoring\n",
                   port->index + 1);
            return false;
        case SATA_SIG_PM:
            printk(LOGLEVEL_INFO,
                   "ahci: port #%" PRIu8 " is a pm device. ignoring\n",
                   port->index + 1);
            return false;
    }

    verify_not_reached();
}

__optimize(3) bool ahci_spec_hba_port_init(struct ahci_hba_port *const port) {
    if (!ahci_hba_port_stop_running(port)) {
        printk(LOGLEVEL_WARN,
               "ahci: failed to stop port #%" PRIu8 " before init\n",
               port->index + 1);
        return false;
    }

    port->sig = (enum sata_sig)mmio_read(&port->spec->signature);
    if (!recognize_port_sig(port)) {
        return false;
    }

    struct page *cmd_list_page = NULL;
    struct page *cmd_table_pages = NULL;

    const bool supports_64bit_dma = ahci_hba_get()->supports_64bit_dma;
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
        return false;
    }

    if (cmd_table_pages == NULL) {
        free_page(cmd_list_page);
        printk(LOGLEVEL_WARN, "ahci: failed to allocate pages for hba port\n");

        return false;
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

    struct ahci_hba_port_cmdhdr_info *const cmdhdr_info_list =
        kmalloc(sizeof(struct ahci_hba_port_cmdhdr_info) *
                AHCI_HBA_CMD_HDR_COUNT);

    if (cmdhdr_info_list == NULL) {
        free_page(cmd_list_page);
        free_pages(cmd_table_pages, AHCI_HBA_CMD_TABLE_PAGE_ORDER);

        return false;
    }

    volatile struct ahci_spec_port_cmdhdr *cmd_header =
        phys_to_virt(cmd_list_phys);
    const volatile struct ahci_spec_port_cmdhdr *const end =
        cmd_header + AHCI_HBA_CMD_HDR_COUNT;

    uint8_t cmdhdr_index = 0;
    for (uint64_t cmdtable_phys = phys_range.front;
         cmd_header != end;
         cmd_header++,
         cmdtable_phys += sizeof(struct ahci_spec_hba_cmd_table),
         cmdhdr_index++)
    {
        mmio_write(&cmd_header->cmd_table_base_lower32,
                   (uint32_t)cmdtable_phys);

        if (supports_64bit_dma) {
            mmio_write(&cmd_header->cmd_table_base_upper32,
                       cmdtable_phys >> 32);
        }

        cmdhdr_info_list[cmdhdr_index] = AHCI_HBA_PORT_CMDHDR_INFO_INIT();
    }

    struct mmio_region *const mmio =
        vmap_mmio(phys_range, PROT_READ | PROT_WRITE, /*flags=*/0);

    if (mmio == NULL) {
        free_page(cmd_list_page);
        free_pages(cmd_table_pages, AHCI_HBA_CMD_TABLE_PAGE_ORDER);

        kfree(cmdhdr_info_list);
        return false;
    }

    port->cmdhdr_info_list = cmdhdr_info_list;
    port->headers = phys_to_virt(cmd_list_phys);
    port->cmdtable_phys = page_to_phys(cmd_table_pages);
    port->mmio = mmio;

    struct page *const resp_page =
        alloc_page(PAGE_STATE_USED, /*alloc_flags=*/0);

    if (resp_page == NULL) {
        free_page(cmd_list_page);
        free_pages(cmd_table_pages, AHCI_HBA_CMD_TABLE_PAGE_ORDER);

        kfree(cmdhdr_info_list);
        return false;
    }

    clear_error_bits(spec);

    ahci_hba_port_power_on_and_spin_up(spec, port->index);
    ahci_hba_port_set_state(spec, AHCI_HBA_PORT_INTERFACE_COMM_CTRL_ACTIVE);

    mmio_write(&spec->cmd_status,
               mmio_read(&spec->cmd_status) |
                __AHCI_HBA_PORT_CMDSTATUS_FIS_RECEIVE_ENABLE);

    flush_writes(spec);
    if (!ahci_hba_port_start(port)) {
        free_page(resp_page);
        return false;
    }

    bool result =
        ahci_hba_port_send_scsi_command(port,
                                        SCSI_REQUEST_IDENTITY(),
                                        page_to_phys(resp_page));

    if (!result) {
        free_page(resp_page);
        printk(LOGLEVEL_WARN,
               "ahci: failed to get identity of port #%" PRIu8 "\n",
               port->index + 1);

        // TODO: Power down and free cmd-list/cmd-table pages
        return false;
    }

    void *const resp_ptr = page_to_virt(resp_page);
    struct atapi_identify *const ident = (struct atapi_identify *)resp_ptr;

    scsi_swap_data(ident->serial, sizeof(ident->serial));
    scsi_swap_data(ident->model, sizeof(ident->model));

    printk(LOGLEVEL_INFO,
           "ahci: identify response:\n"
           "\tdevice-type: 0x%" PRIx16 "\n"
           "\tserial: " SV_FMT "\n"
           "\tmodel: " SV_FMT "\n"
           "\tcapabilities: 0x%" PRIx32 "\n"
           "\t\tdma supported: %s\n"
           "\t\tlba supported: %s\n"
           "\t\tiordy supported: %s\n"
           "\t\tstandy timer supported: %s\n",
           ident->device_type,
           SV_FMT_ARGS(sv_of_carr(ident->serial)),
           SV_FMT_ARGS(sv_of_carr(ident->model)),
           ident->capabilities,
           ident->capabilities & __ATA_IDENTITY_CAP_DMA_SUPPORT ? "yes" : "no",
           ident->capabilities & __ATA_IDENTITY_CAP_LBA_SUPPORT ? "yes" : "no",
           ident->capabilities & __ATA_IDENTITY_CAP_IORDY_SUPPORT ?
            "yes" : "no",
           ident->capabilities & __ATA_IDENTITY_CAP_STANDBY_TIMER_SUPPORTED ?
            "yes" : "no");

    printk(LOGLEVEL_INFO,
           "ahci: port #%" PRIu8 " initialized\n",
           port->index + 1);

    result =
        ahci_hba_port_send_scsi_command(port,
                                        SCSI_REQUEST_READ(0, 1),
                                        page_to_phys(resp_page));

    if (!result) {
        printk(LOGLEVEL_WARN,
               "ahci: failed to read sector 0 of port #%" PRIu8 "\n",
               port->index + 1);

        // TODO: Power down and free cmd-list/cmd-table pages
        return;
    }

    printk(LOGLEVEL_INFO,
           "ahci: magic is 0x%" PRIx16 "\n",
           *(uint16_t *)resp_ptr);

    free_page(resp_page);
    return true;
}

__optimize(3)
static void print_interrupt_status(const uint32_t interrupt_status) {
    enum ahci_hba_port_interrupt_status_flags flag =
        __AHCI_HBA_IS_DEV_TO_HOST_FIS;

    if (interrupt_status == 0) {
        printk(LOGLEVEL_WARN, "ahci: interrupt status is 0\n");
        return;
    }

    switch (flag) {
        case __AHCI_HBA_IS_DEV_TO_HOST_FIS:
        case __AHCI_HBA_IS_PIO_SETUP_FIS:
        case __AHCI_HBA_IS_DMA_SETUP_FIS:
        case __AHCI_HBA_IS_SET_DEV_BITS_FIS:
        case __AHCI_HBA_IS_DESC_PROCESSED:
            [[fallthrough]];

        #define ERROR_CASE(flag, msg, ...) \
            case flag: \
                do { \
                    if (interrupt_status & flag) { \
                        printk(LOGLEVEL_WARN, msg "\n", ##__VA_ARGS__); \
                    } \
                } while (false)

        ERROR_CASE(__AHCI_HBA_IS_UNKNOWN_FIS, "ahci: got unknown-fis error");
            [[fallthrough]];
        ERROR_CASE(__AHCI_HBA_IS_PORT_CHANGE, "ahci: got port-change error");
            [[fallthrough]];
        ERROR_CASE(__AHCI_HBA_IS_DEV_MECH_PRESENCE, "ahci: got dev-mech error");
            [[fallthrough]];
        ERROR_CASE(__AHCI_HBA_IS_PHYRDY_CHANGE_STATUS,
                   "ahci: got phyrdy change error");
            [[fallthrough]];
        ERROR_CASE(__AHCI_HBA_IS_INCORRECT_PORT_MULT_STATUS,
                   "ahci: got incorrect port-mult error");
            [[fallthrough]];
        ERROR_CASE(__AHCI_HBA_IS_OVERFLOW_STATUS, "ahci: got overflow error");
            [[fallthrough]];
        ERROR_CASE(__AHCI_HBA_IS_INTERFACE_NOT_FATAL_ERR_STATUS,
                   "ahci: got interface not-fatal error");
            [[fallthrough]];
        ERROR_CASE(__AHCI_HBA_IS_INTERFACE_FATAL_ERR_STATUS,
                   "ahci: got unknown-fis error");
            [[fallthrough]];
        ERROR_CASE(__AHCI_HBA_IS_HOST_BUS_DATA_ERR_STATUS,
                   "ahci: got host-bus data error");
            [[fallthrough]];
        ERROR_CASE(__AHCI_HBA_IS_HOST_BUS_FATAL_ERR_STATUS,
                   "ahci: got host bus fatal error");
            [[fallthrough]];
        ERROR_CASE(__AHCI_HBA_IS_TASK_FILE_ERR_STATUS,
                   "ahci: got task file error");
            [[fallthrough]];
        ERROR_CASE(__AHCI_HBA_IS_COLD_PORT_DETECT_STATUS,
                   "ahci: got cold-port detect error");
            return;

        #undef ERROR_CASE
    }

    verify_not_reached();
}

__optimize(3)
static inline bool fix_any_errors(struct ahci_hba_port *const port) {
    switch (port->state) {
        case AHCI_HBA_PORT_STATE_OK:
            return true;
        case AHCI_HBA_PORT_STATE_NEEDS_RESET:
            if (reset_port(port)) {
                port->state = AHCI_HBA_PORT_STATE_OK;
                return true;
            }

            return false;
    }

    verify_not_reached();
}

__optimize(3)
static inline uint8_t prepare_port(struct ahci_hba_port *const port) {
    if (!fix_any_errors(port)) {
        return UINT8_MAX;
    }

    if (!ahci_hba_port_stop_running(port)) {
        printk(LOGLEVEL_WARN,
               "ahci: failed to stop port #%" PRIu8 "\n",
               port->index + 1);
        return UINT8_MAX;
    }

    const uint8_t slot = find_free_cmdhdr(port);
    if (slot == UINT8_MAX) {
        printk(LOGLEVEL_WARN,
               "ahci-port: port #%" PRIu8 " has no free command-headers\n",
               port->index + 1);
        return UINT8_MAX;
    }

    // Clear any pending interrupts
    mmio_write(&port->spec->interrupt_status, 0);
    return slot;
}

__optimize(3) static void
setup_ata_h2d_fis(struct ahci_spec_hba_cmd_table *const cmd_table,
                  const uint8_t command,
                  const uint8_t feature,
                  const uint64_t sector_offset,
                  const uint8_t sector_count)
{
    struct ahci_spec_fis_reg_h2d *const h2d_fis =
        (struct ahci_spec_fis_reg_h2d *)(uint64_t)cmd_table->command_fis;

    h2d_fis->fis_type = AHCI_FIS_KIND_REG_H2D;
    h2d_fis->flags = __AHCI_FIS_REG_H2D_IS_ATA_CMD;

    h2d_fis->command = command;
    h2d_fis->feature_low8 = feature;

    h2d_fis->lba0 = (uint8_t)sector_offset;
    h2d_fis->lba1 = (uint8_t)(sector_offset >> 8);
    h2d_fis->lba2 = (uint8_t)(sector_offset >> 16);

    h2d_fis->device = __ATA_USE_LBA_ADDRESSING;

    h2d_fis->lba3 = (uint8_t)(sector_offset >> 24);
    h2d_fis->lba4 = (uint8_t)(sector_offset >> 32);
    h2d_fis->lba5 = (uint8_t)(sector_offset >> 40);

    h2d_fis->feature_high8 = 0;
    h2d_fis->count_low = sector_count;
    h2d_fis->count_high = sector_count >> 8;
    h2d_fis->icc = 0;
    h2d_fis->control = 0;
}

__optimize(3) static void
setup_prdt_table(volatile struct ahci_spec_port_cmdhdr *const cmd_header,
                 struct ahci_spec_hba_cmd_table *const cmd_table,
                 const uint64_t phys_addr,
                 const uint32_t sector_count,
                 const uint32_t flags)
{
    mmio_write(&cmd_header->flags, flags);

    volatile struct ahci_spec_hba_prdt_entry *const entries =
        cmd_table->prdt_entries;

    const uint32_t byte_count = check_mul_assert(sector_count, SECTOR_SIZE);
    const uint32_t prdt_count =
        div_round_up(byte_count, AHCI_HBA_PORT_MAX_COUNT);

    mmio_write(&cmd_header->prdt_length, prdt_count);
    uint64_t response_addr = phys_addr;

    const uint32_t entry_flags = AHCI_HBA_PORT_MAX_COUNT - 1;
    const bool supports_64bit_dma = ahci_hba_get()->supports_64bit_dma;

    for (uint8_t i = 0;
         i < prdt_count - 1;
         i++, response_addr += AHCI_HBA_PORT_MAX_COUNT)
    {
        volatile struct ahci_spec_hba_prdt_entry *const entry = &entries[i];

        mmio_write(&entry->data_base_address_lower32, (uint32_t)response_addr);
        if (supports_64bit_dma) {
            mmio_write(&entry->data_base_address_upper32, response_addr >> 32);
        }

        mmio_write(&entry->flags, entry_flags);
    }

    volatile struct ahci_spec_hba_prdt_entry *const last_entry =
        &entries[prdt_count - 1];

    const uint32_t last_entry_byte_count = byte_count % AHCI_HBA_PORT_MAX_COUNT;
    uint32_t last_entry_flags = 0;

    if (last_entry_byte_count != 0) {
        last_entry_flags = last_entry_byte_count - 1;
    } else {
        last_entry_flags = AHCI_HBA_PORT_MAX_COUNT - 1;
    }

    mmio_write(&last_entry->data_base_address_lower32, (uint32_t)response_addr);
    if (supports_64bit_dma) {
        mmio_write(&last_entry->data_base_address_upper32, response_addr >> 32);
    }

    mmio_write(&last_entry->flags, last_entry_flags);
}

static bool
send_ata_command(struct ahci_hba_port *const port,
                 const enum ata_command command_kind,
                 const uint64_t sector_offset,
                 const uint16_t sector_count,
                 const uint64_t phys_addr)
{
    if (__builtin_expect(sector_count == 0, 0)) {
        printk(LOGLEVEL_WARN,
               "ahci-port: port #%" PRIu8 " got a h2d request of 0 sectors\n",
               port->index + 1);
        return true;
    }

    const uint8_t slot = prepare_port(port);
    if (slot == UINT8_MAX) {
        return false;
    }

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

    volatile struct ahci_spec_port_cmdhdr *const cmd_header =
        &port->headers[slot];
    struct ahci_spec_hba_cmd_table *const cmd_table =
        (struct ahci_spec_hba_cmd_table *)
            phys_to_virt(port->cmdtable_phys) + slot;

    setup_prdt_table(cmd_header, cmd_table, phys_addr, sector_count, flags);
    setup_ata_h2d_fis(cmd_table,
                      command_kind,
                      /*feature=*/0,
                      sector_offset,
                      sector_count);

    if (!wait_for_tfd_idle(port) || !ahci_hba_port_start_running(port)) {
        return false;
    }

    struct event *const event = &port->cmdhdr_info_list[slot].event;
    struct await_result await_result = AWAIT_RESULT_NULL();

    mmio_write(&port->spec->command_issue, 1ull << slot);
    assert(
        events_await(&event,
                     /*event_count=*/1,
                     /*block=*/true,
                     &await_result) == 0);

    if (!await_result.result_bool) {
        print_interrupt_status(port->error.interrupt_status);

        print_serr_error(port->error.serr);
        print_serr_diag(port->error.serr);
    }

    return await_result.result_bool;
}

__optimize(3) static void
setup_atapi_command(volatile uint8_t *const atapi_command,
                    const enum atapi_command command,
                    const uint64_t sector_offset,
                    const uint8_t sector_count)
{
    atapi_command[0] = command;
    atapi_command[1] = 0;

    atapi_command[2] = (sector_offset >> 24) & 0xFF;
    atapi_command[3] = (sector_offset >> 16) & 0xFF;
    atapi_command[4] = (sector_offset >> 8) & 0xFF;
    atapi_command[5] = sector_offset & 0xFF;

    atapi_command[6] = (sector_count >> 24) & 0xFF;
    atapi_command[7] = (sector_count >> 16) & 0xFF;
    atapi_command[8] = (sector_count >> 8) & 0xFF;
    atapi_command[9] = sector_count & 0xFF;
}

static bool
send_atapi_command(struct ahci_hba_port *const port,
                   const enum atapi_command command_kind,
                   const uint64_t sector_offset,
                   const uint16_t sector_count,
                   const uint64_t phys_addr)
{
    if (__builtin_expect(sector_count == 0, 0)) {
        printk(LOGLEVEL_WARN,
               "ahci-port: port #%" PRIu8 " got a h2d request of 0 sectors\n",
               port->index + 1);
        return true;
    }

    const uint8_t slot = prepare_port(port);
    if (slot == UINT8_MAX) {
        return false;
    }

    uint16_t flags =
        (sizeof(struct ahci_spec_fis_reg_h2d) / sizeof(uint32_t)) |
        __AHCI_PORT_CMDHDR_ATAPI;

    volatile struct ahci_spec_port_cmdhdr *const cmd_header =
        &port->headers[slot];
    struct ahci_spec_hba_cmd_table *const cmd_table =
        (struct ahci_spec_hba_cmd_table *)
            phys_to_virt(port->cmdtable_phys) + slot;

    setup_prdt_table(cmd_header, cmd_table, phys_addr, sector_count, flags);
    setup_ata_h2d_fis(cmd_table,
                      ATA_CMD_PACKET,
                      __AHCI_FIS_REG_H2D_FEAT_ATAPI_DMA,
                      /*sector_offset=*/0,
                      /*sector_count=*/0);

    volatile uint8_t *const atapi_cmd = cmd_table->atapi_command;
    setup_atapi_command(atapi_cmd, command_kind, sector_offset, sector_count);

    if (!wait_for_tfd_idle(port) || !ahci_hba_port_start_running(port)) {
        return false;
    }

    struct event *const event = &port->cmdhdr_info_list[slot].event;
    struct await_result await_result = AWAIT_RESULT_NULL();

    mmio_write(&port->spec->command_issue, 1ull << slot);
    assert(
        events_await(&event,
                     /*event_count=*/1,
                     /*block=*/true,
                     &await_result) == 0);

    if (!await_result.result_bool) {
        print_interrupt_status(port->error.interrupt_status);

        print_serr_error(port->error.serr);
        print_serr_diag(port->error.serr);
    }

    return await_result.result_bool;
}

bool
ahci_hba_port_send_scsi_command(struct ahci_hba_port *const port,
                                const struct scsi_request request,
                                const uint64_t phys_addr)
{
    switch (request.command) {
        case SCSI_CMD_IDENTIFY: {
            switch (port->sig) {
                case SATA_SIG_ATA:
                    return send_ata_command(port,
                                            ATA_CMD_IDENTIFY,
                                            /*sector_offset=*/0,
                                            /*sector_count=*/1,
                                            phys_addr);
                case SATA_SIG_ATAPI:
                    return send_ata_command(port,
                                            ATA_CMD_IDENTIFY_PACKET,
                                            /*sector_offset=*/0,
                                            /*sector_count=*/1,
                                            phys_addr);
                case SATA_SIG_SEMB:
                case SATA_SIG_PM:
                    verify_not_reached();
            }

            verify_not_reached();
        }
        case SCSI_CMD_READ:
            switch (port->sig) {
                case SATA_SIG_ATA:
                    return send_ata_command(port,
                                            ATA_CMD_READ_DMA_EXT,
                                            request.read.position,
                                            request.read.length,
                                            phys_addr);
                case SATA_SIG_ATAPI:
                    return send_atapi_command(port,
                                              ATAPI_CMD_READ,
                                              request.read.position,
                                              request.read.length,
                                              phys_addr);
                case SATA_SIG_SEMB:
                case SATA_SIG_PM:
                    verify_not_reached();
            }

            verify_not_reached();
        case SCSI_CMD_WRITE:
            switch (port->sig) {
                case SATA_SIG_ATA:
                    return send_ata_command(port,
                                            ATA_CMD_WRITE_DMA_EXT,
                                            request.read.position,
                                            request.read.length,
                                            phys_addr);
                case SATA_SIG_ATAPI:
                    return send_atapi_command(port,
                                              ATAPI_CMD_WRITE,
                                              request.read.position,
                                              request.read.length,
                                              phys_addr);
                case SATA_SIG_SEMB:
                case SATA_SIG_PM:
                    verify_not_reached();
            }

            verify_not_reached();
    }

    verify_not_reached();
}