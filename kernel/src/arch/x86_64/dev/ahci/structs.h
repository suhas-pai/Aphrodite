/*
 * kernel/src/arch/x86_64/dev/ahci/structs.h
 * © suhas pai
 */

#include <stdint.h>
#include "lib/macros.h"

struct ahci_spec_hba_port {
    volatile uint32_t cmd_list_base_phys_lower32; // 1K byte-aligned
    volatile uint32_t cmd_list_base_phys_upper32;
    volatile uint32_t fis_base_address_lower32; // 256-byte aligned
    volatile uint32_t fis_base_address_upper32;
    volatile uint32_t interrupt_status;
    volatile uint32_t interrupt_enable;
    volatile uint32_t command_and_status;
    volatile uint32_t reserved;
    volatile uint32_t task_file_data;
    volatile uint32_t signature;
    volatile uint32_t sata_status;
    volatile uint32_t sata_control;
    volatile uint32_t sata_error;
    volatile uint32_t sata_active;
    volatile uint32_t command_issue;
    volatile uint32_t sata_notification;
    volatile uint32_t fis_switch_control;
    volatile uint32_t reserved_1[11];
    volatile uint32_t vendor[4];
};

struct ahci_spec_hba_registers {
    volatile const uint32_t host_capabilities;

    volatile uint32_t global_host_control;
    volatile uint32_t interrupt_status;

    volatile const uint32_t port_implemented;
    volatile const uint32_t version;

    volatile uint32_t command_completion_coalescing_control;
    volatile uint32_t command_completion_coalescing_ports;

    volatile const uint32_t enclosure_management_location;
    volatile uint32_t enclosure_management_control;

    volatile const uint32_t host_capabilities_extended;
    volatile uint32_t bios_os_handoff_ctrl_and_status;

    volatile uint8_t reserved[0xA0-0x2C];
    volatile uint8_t vendor_registers[0x100-0xA0];

    volatile struct ahci_spec_hba_port ports[32];
};

enum ahci_hba_interface_speed_support {
    AHCI_HBA_INTERFACE_SPEED_GEN1 = 1, // 1.5 gbps
    AHCI_HBA_INTERFACE_SPEED_GEN2, // 3 gbps
    AHCI_HBA_INTERFACE_SPEED_GEN3,// 6 gbps
};

enum ahci_hba_host_capability_shifts {
    AHCI_HBA_HOST_CAP_SHIFT_CMD_SLOT_COUNT_SHIFT = 8,
    AHCI_HBA_HOST_CAP_INTERFACE_SPEED_SUPPORT_SHIFT = 20
};

enum ahci_hba_host_capability_flags {
    __AHCI_HBA_HOST_CAP_PORTS_IMPLED = 0b11111,
    __AHCI_HBA_HOST_CAP_SUPPORTS_EXT_SATA = 1ull << 5,
    __AHCI_HBA_HOST_CAP_SUPPORTS_EMS = 1ull << 6,
    __AHCI_HBA_HOST_CAP_SUPPORTS_CCCS = 1ull << 7,
    __AHCI_HBA_HOST_CAP_CMD_SLOTS_COUNT =
        0b11111ull << AHCI_HBA_HOST_CAP_SHIFT_CMD_SLOT_COUNT_SHIFT,

    __AHCI_HBA_HOST_CAP_PARTIAL_STATE_CAPABLE = 1ull << 13,
    __AHCI_HBA_HOST_CAP_SLUMBER_STATE_CAPABLE = 1ull << 14,
    __AHCI_HBA_HOST_CAP_PIQ_MULT_DRQ_BLOCK = 1ull << 15,
    __AHCI_HBA_HOST_CAP_FIS_SWITCH_SUPPORT = 1ull << 16,
    __AHCI_HBA_HOST_CAP_SUPPORTS_PORT_MULTIPLIER = 1ull << 17,
    __AHCI_HBA_HOST_CAP_SUPPORTS_AHCI_MODE_ONLY = 1ull << 18,
    __AHCI_HBA_HOST_CAP_INTERFACE_SPEED_SUPPORT =
        0b1111 << AHCI_HBA_HOST_CAP_INTERFACE_SPEED_SUPPORT_SHIFT,

    __AHCI_HBA_HOST_CAP_SUPPORTS_CMD_LIST_OVERRIDE = 1ull << 24,
    __AHCI_HBA_HOST_CAP_SUPPORTS_ACTIVITY_LED = 1ull << 25,
    __AHCI_HBA_HOST_CAP_SUPPORTS_AGGRESSIVE_LINK_PWR_MGMT = 1ull << 26,
    __AHCI_HBA_HOST_CAP_SUPPORTS_STAGGERED_SPINUP = 1ull << 27,
    __AHCI_HBA_HOST_CAP_SUPPORTS_MECH_PRESENCE_SWITCH = 1ull << 28,
    __AHCI_HBA_HOST_CAP_SUPPORTS_NOTIF_REG = 1ull << 29,
    __AHCI_HBA_HOST_CAP_NATIVE_CMD_QUEUE = 1ull << 30,
    __AHCI_HBA_HOST_CAP_64BIT_DMA = 1ull << 31,
};

enum ahci_hba_global_host_control_flags {
    /*
     * When set by SW, this bit causes an internal reset of the HBA.
     *
     * All state machines that relate to data transfers and queuing shall return
     * to an idle condition, and all ports shall be re-initialized via COMRESET
     * (if staggered spin-up is not supported). If staggered spin-up is
     * supported, then it is the responsibility of software to spin-up each port
     * after the reset has completed.
     *
     * When the HBA has performed the reset action, it shall reset this bit to
     * ‘0’. A software write of ‘0’ shall have no effect. For a description on
     * which bits are reset when this bit is set, see see section 10.4.3.
     */
    __AHCI_HBA_GLOBAL_HOST_CTRL_HBA_RESET = 1ull << 0,
    __AHCI_HBA_GLOBAL_HOST_CTRL_INT_ENABLE = 1ull << 1,
    __AHCI_HBA_GLOBAL_HOST_CTRL_MSI_REVERT_TO_SINGLE_MEASURE = 1ull << 2,
    __AHCI_HBA_GLOBAL_HOST_CTRL_AHCI_ENABLE = 1ull << 31,
};

enum ahci_hba_cmd_compl_coalescing_ctrl_shifts {
    AHCI_HBA_CMD_COMPL_COALESCING_INT_NUMBER_SHIFT = 3,
    AHCI_HBA_CMD_COMPL_COALESCING_COUNT_REQ_FOR_INT_SHIFT = 8,
    AHCI_HBA_CMD_COMPL_COALESCING_TIMEOUT_VALUE_MS_SHIFT = 16
};

enum ahci_hba_cmd_compl_coalescing_ctrl_flags {
    __AHCI_HBA_CMD_COMPL_COALESCING_CTRL_ENABLE = 1ull << 0,

    /*
     * Specifies the interrupt used by the CCC feature. This interrupt must be
     * marked as unused in the Ports Implemented (PI) register by the
     * corresponding bit being set to ‘0’. Thus, the CCC interrupt corresponds
     * to the interrupt for an unimplemented port on the controller. When a CCC
     * interrupt occurs, the IS.IPS[INT] bit shall be asserted to ‘1’. This
     * field also specifies the interrupt vector used for MSI.
     */
    __AHCI_HBA_CMD_COMPL_COALESCING_INT_NUMBER =
        0b11111 << AHCI_HBA_CMD_COMPL_COALESCING_INT_NUMBER_SHIFT,

    /*
     * Specifies the number of command completions that are necessary to cause a
     * CCC interrupt. The HBA has an internal command completion counter,
     * hCccComplete. hCccComplete is incremented by one each time a selected
     * port has a command completion. When hCccComplete is equal to the command
     * completions value, a CCC interrupt is signaled. The internal command
     * completion counter is reset to ‘0’ on the assertion of each CCC
     * interrupt. A value of ‘0’ for this field shall disable CCC interrupts
     * being generated based on the number of commands completed, i.e. CCC
     * interrupts are only generated based on the timer in this case.
     */
    __AHCI_HBA_CMD_COMPL_COALESCING_COUNT_REQ_FOR_INT =
        0xffull << AHCI_HBA_CMD_COMPL_COALESCING_COUNT_REQ_FOR_INT_SHIFT,

    /*
     * The timeout value is specified in 1 millisecond intervals. The timer
     * accuracy shall be within 5%. hCccTimer is loaded with this timeout value.
     *
     * hCccTimer is only decremented when commands are outstanding on selected
     * ports, as defined in section 11.2. The HBA will signal a CCC interrupt
     * when hCccTimer has decremented to ‘0’.
     *
     * hCccTimer is reset to the timeout value on the assertion of each CCC
     * interrupt. A timeout value of ‘0’ is reserved
     */
    __AHCI_HBA_CMD_COMPL_COALESCING_TIMEOUT_VALUE_MS =
        0xffffull << AHCI_HBA_CMD_COMPL_COALESCING_TIMEOUT_VALUE_MS_SHIFT
};

enum ahci_hba_pio_readterface_comm_ctrl {
    AHCI_HBA_pio_readTERFACE_COMM_CTRL_IDLE,
    AHCI_HBA_pio_readTERFACE_COMM_CTRL_ACTIVE,
    AHCI_HBA_pio_readTERFACE_COMM_CTRL_PATRIAL,
    AHCI_HBA_pio_readTERFACE_COMM_CTRL_SLUMBER = 6,
    AHCI_HBA_pio_readTERFACE_COMM_CTRL_DEV_SLEEP = 8
};

enum ahci_hba_port_cmd_status_shifts {
    AHCI_HBA_PORT_CMDSTATUS_CURRENT_CMD_SLOT_SHIFT = 8,
};

enum ahci_hba_port_cmd_status_flags {
    __AHCI_HBA_PORT_CMDSTATUS_START = 1ull << 0,

    /*
     * This bit is read/write for HBAs that support staggered spin-up
     * via CAP.SSS. This bit is read only ‘1’ for HBAs that do not support
     * staggered spin-up.
     */
    __AHCI_HBA_PORT_CMDSTATUS_SPINUP_DEVICE = 1ull << 1,

    /*
     * This bit is read/write for HBAs that support cold presence
     * detection on this port as indicated by PxCMD.CPD set to ‘1’.
     * This bit is read only ‘1’ for HBAs that do not support cold presence
     * detect.
     */
    __AHCI_HBA_PORT_CMDSTATUS_POWERON_DEVICE = 1ull << 2,
    __AHCI_HBA_PORT_CMDSTATUS_CMD_LIST_OVERRIDE = 1ull << 3,
    __AHCI_HBA_PORT_CMDSTATUS_FIS_RECEIVE_ENABLE = 1ull << 4,
    __AHCI_HBA_PORT_CMDSTATUS_CURRENT_CMD_SLOT =
        0b11111ull << AHCI_HBA_PORT_CMDSTATUS_CURRENT_CMD_SLOT_SHIFT,

    __AHCI_HBA_PORT_CMDSTATUS_MECH_PRESENCE_SWITCH_STATE = 1ull << 13,
    __AHCI_HBA_PORT_CMDSTATUS_FIS_RECEIVE_RUNNING = 1ull << 14,
    __AHCI_HBA_PORT_CMDSTATUS_CMD_LIST_RUNNING = 1ull << 15,
    __AHCI_HBA_PORT_CMDSTATUS_COLD_PRESENCE_STATE = 1ull << 16,
    __AHCI_HBA_PORT_CMDSTATUS_PORT_MULTIPLIER_ATTACHED = 1ull << 17,
    __AHCI_HBA_PORT_CMDSTATUS_HOT_PLUG_CAPABLE = 1ull << 18,
    __AHCI_HBA_PORT_CMDSTATUS_MECH_PRESENCE_SWITCH_ATTACHED = 1ull << 19,
    __AHCI_HBA_PORT_CMDSTATUS_COLD_PRESENCE_DETECTION = 1ull << 20,
    __AHCI_HBA_PORT_CMDSTATUS_EXTERN_SATA_ACCESSIBLE = 1ull << 21,
    __AHCI_HBA_PORT_CMDSTATUS_FIS_SWITCH_CAPABLE = 1ull << 22,
    __AHCI_HBA_PORT_CMDSTATUS_AUTO_PARTIAL_TO_SLUMBER_ENABLED = 1ull << 23,
    __AHCI_HBA_PORT_CMDSTATUS_ATAPI_DEVICE = 1ull << 24,
    __AHCI_HBA_PORT_CMDSTATUS_DRIVE_LED_ACTIVE_ON_ATAPI_ENABLE = 1ull << 25,
    __AHCI_HBA_PORT_CMDSTATUS_AGGRESIVE_LINK_PWR_MGMT_ENABLE = 1ull << 26,
    __AHCI_HBA_PORT_CMDSTATUS_AGGRESIVE_SLUMBER_PARTIAL = 1ull << 27,
    __AHCI_HBA_PORT_CMDSTATUS_INTERFACE_COMM_CTRL = 0b11111ull << 28,
};

enum ahci_hba_pio_readterrupt_enable_flags {
    __AHCI_HBA_IE_DEV_TO_HOST_FIS_INT_ENABLE = 1ull << 0,
    __AHCI_HBA_IE_PIO_SETUP_FIS_INT_ENABLE = 1ull << 1,
    __AHCI_HBA_IE_DMA_SETUP_FIS_INT_ENABLE = 1ull << 2,
    __AHCI_HBA_IE_SET_DEV_BITS_FIS_INT_ENABLE = 1ull << 3,
    __AHCI_HBA_IE_UNKNOWN_FIS_INT_ENABLE = 1ull << 4,
    __AHCI_HBA_IE_DESC_PROCESSED_INT_ENABLE = 1ull << 5,
    __AHCI_HBA_IE_PORT_CHANGE_INT_ENABLE = 1ull << 6,
    __AHCI_HBA_IE_DEV_MECH_PRESENCE_INT_ENABLE = 1ull << 7,
    __AHCI_HBA_IE_PHYRDY_CHANGE_STATUS = 1ull << 22,
    __AHCI_HBA_IE_INCORRECT_PORT_MULT_STATUS = 1ull << 23,
    __AHCI_HBA_IE_OVERFLOW_STATUS = 1ull << 24,
    __AHCI_HBA_IE_INTERFACE_NOT_FATAL_ERR_STATUS = 1ull << 26,
    __AHCI_HBA_IE_INTERFACE_FATAL_ERR_STATUS = 1ull << 27,
    __AHCI_HBA_IE_HOST_BUS_DATA_ERR_STATUS = 1ull << 28,
    __AHCI_HBA_IE_HOST_BUS_FATAL_ERR_STATUS = 1ull << 29,
    __AHCI_HBA_IE_TASK_FILE_ERR_STATUS = 1ull << 30,
    __AHCI_HBA_IE_COLD_PORT_DETECT_STATUS = 1ull << 31,
};

#define SATA_SIG_ATA 0x00000101 // SATA drive
#define SATA_SIG_ATAPI 0xEB140101 // SATAPI drive
#define SATA_SIG_SEMB 0xC33C0101 // Enclosure management bridge
#define SATA_SIG_PM 0x96690101  // Port multiplier

#define AHCI_DEV_NULL 0
#define AHCI_DEV_SATA 1
#define AHCI_DEV_SEMB 2
#define AHCI_DEV_PM 3
#define AHCI_DEV_SATAPI 4

enum ahci_hba_port_ipm {
    AHCI_HBA_PORT_IPM_ACTIVE = 1,
    AHCI_HBA_PORT_IPM_PARTIAL_POWER_MGMT_STATE,
    AHCI_HBA_PORT_IPM_PARTIAL_SLUMBER_MGMT_STATE,
    AHCI_HBA_PORT_IPM_PARTIAL_DEVSLEEP_MGMT_STATE
};

enum ahci_hba_port_det {
    AHCI_HBA_PORT_DET_PRESENT_BUT_NO_COMM = 1,
    AHCI_HBA_PORT_DET_PRESENT = 3,
    AHCI_HBA_PORT_DET_OFFLINE_MODE
};

struct ahci_spec_hba_prdt_entry {
    volatile uint32_t data_base_address_lower32;
    volatile uint32_t data_base_address_upper32;
    volatile uint32_t reserved;

    volatile uint32_t data_byte_count:22; // 4M max
    volatile uint32_t reserved_1:9;
    volatile uint32_t interrupt_on_completion:1;
};

#define AHCI_HBA_MAX_PRDT_ENTRIES 8

struct ahci_spec_hba_cmd_table {
    volatile uint8_t command_fis[64];
    volatile uint8_t atapi_command[16];
    volatile uint8_t reserved[48];

    volatile struct ahci_spec_hba_prdt_entry prdt_entries[AHCI_HBA_MAX_PRDT_ENTRIES];
};

enum ahci_fis_kind {
    AHCI_FIS_KIND_REG_H2D = 0x27,
    AHCI_FIS_KIND_REG_D2H = 0x34,
    AHCI_FIS_KIND_DMA_ACT = 0x39,
    AHCI_FIS_KIND_DMA_SETUP = 0x41,
    AHCI_FIS_KIND_DATA = 0x46,
    AHCI_FIS_KIND_BIST = 0x58,
    AHCI_FIS_KIND_PIO_SETUP = 0x5F,
    AHCI_FIS_KIND_DEV_BITS = 0xA1,
};

enum ahci_fis_reg_h2d_flags {
    __AHCI_FIS_REG_H2D_IS_ATA_CMD = 1 << 7
};

struct ahci_spec_fis_reg_h2d {
    uint8_t fis_type;
    uint8_t flags;

    uint8_t command;
    uint8_t feature_low8;

    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;

    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t feature_high8;

    uint8_t countl;
    uint8_t counth;
    uint8_t icc;
    uint8_t control;

    uint8_t reserved_1[4];
} __packed;

struct ahci_spec_fis_reg_d2h {
    uint8_t fis_type;
    uint8_t pmport:4;
    uint8_t reserved:2;
    uint8_t i:1;
    uint8_t rsv1:1;

    uint8_t status;
    uint8_t error;

    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;

    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t reserved_2;

    uint8_t countl;
    uint8_t counth;

    uint8_t reserved_3[2];
    uint8_t reserved_4[4];
} __packed;

struct ahci_spec_fis_data {
    uint8_t fis_type;
    uint8_t pmport:4;
    uint8_t reserved:4;

    uint8_t reserved_1[2];
    uint32_t data[1];
} __packed;

struct ahci_spec_fis_pio_setup {
    uint8_t fis_type;
    uint8_t pmport:4;
    uint8_t reserved:1;
    uint8_t d:1;
    uint8_t i:1;
    uint8_t reserved_1:1;

    uint8_t status;
    uint8_t error;

    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;

    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t reserved_2;

    uint8_t countl;
    uint8_t counth;
    uint8_t reserved_3;
    uint8_t e_status;

    uint16_t tc;
    uint8_t rsv4[2];
} __packed;

struct ahci_spec_fis_dma_setup {
    uint8_t fis_type;
    uint8_t pmport:4;
    uint8_t reserved:1;
    uint8_t d:1;
    uint8_t i:1;
    uint8_t a:1;

    uint8_t reserved_1[2];

    uint64_t DMAbufferID;
    uint32_t rsvd;
    uint32_t DMAbufOffset;
    uint32_t TransferCount;
    uint32_t reserved_2;
} __packed;

struct ahci_spec_hba_fis {
    struct ahci_spec_fis_dma_setup dma_fis;
    uint8_t pad0[4];

    struct ahci_spec_fis_pio_setup pio_fis;
    uint8_t pad1[12];

    struct ahci_spec_fis_reg_d2h reg_d2g;
    uint8_t pad2[4];

    uint32_t sdbfis;

    uint8_t ufis[64];
    uint8_t reserved[0x100-0xA0];
} __packed;

enum ahci_port_command_header_shifts {
    AHCI_PORT_CMDHDR_PORT_MULT_PORT_SHIFT = 12,
    AHCI_PORT_CMDHDR_PRDT_LENGTH_SHIFT = 15
};

enum ahci_port_command_header_flags {
    __AHCI_PORT_CMDHDR_FIS_LENGTH = 0b11111,

    __AHCI_PORT_CMDHDR_ATAPI = 1 << 5,
    __AHCI_PORT_CMDHDR_WRITE = 1 << 6,
    __AHCI_PORT_CMDHDR_PREFETCHABLE = 1 << 7,
    __AHCI_PORT_CMDHDR_RESET = 1 << 8,
    __AHCI_PORT_CMDHDR_BIST = 1 << 9,
    __AHCI_PORT_CMDHDR_CLR_BUSY_ON_ROK = 1 << 10,

    __AHCI_PORT_CMDHDR_PORT_MULT_PORT =
        0b1111 << AHCI_PORT_CMDHDR_PORT_MULT_PORT_SHIFT,
    __AHCI_PORT_CMDHDR_PRDT_LENGTH =
        0xffffull << AHCI_PORT_CMDHDR_PRDT_LENGTH_SHIFT,
};

struct ahci_spec_port_cmd_header {
    volatile uint16_t flags;

    uint16_t prdt_length;
    uint32_t prd_byte_count;
    uint32_t cmd_table_base_lower32;
    uint32_t cmd_table_base_upper32;

    uint32_t reserved[4];
} __packed;

#define AHCI_HBA_REGS_BAR_INDEX 5
