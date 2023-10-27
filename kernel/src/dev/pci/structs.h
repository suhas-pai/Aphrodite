/*
 * kernel/dev/pci/structs.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "lib/macros.h"

#define PCI_MAX_BUS_COUNT 255 // Maximum number of PCI Buses
#define PCI_MAX_DEVICE_COUNT 32 // Each bus has upto 32 devices
#define PCI_MAX_FUNCTION_COUNT 8 // Each device has upto 8 functions

#define PCI_BAR_COUNT_FOR_GENERAL 6
#define PCI_BAR_COUNT_FOR_BRIDGE 2

#define PCI_READ_FAIL (uint32_t)-1

enum pci_spec_device_header_kind {
    PCI_SPEC_DEVHDR_KIND_GENERAL,
    PCI_SPEC_DEVHDR_KIND_PCI_BRIDGE,
    PCI_SPEC_DEVHDR_KIND_CARDBUS_BRIDGE,
};

enum pci_spec_device_header_kind_flags {
    __PCI_DEVHDR_MULTFUNC = 1ull << 7,
};

enum pci_spec_device_bist_flags {
    __PCI_DEVBIST_COMPLETION_CODE = 0b1111,
    __PCI_DEVBIST_ENABLE = 1ull << 6,
    __PCI_DEVBIST_CAPABLE = 1ull << 7,
};

enum pci_spec_device_command_register_flags {
    /*
     * If set to 1 the device can respond to I/O Space accesses; otherwise, the
     * device's response is disabled.
     */

    __PCI_DEVCMDREG_IOSPACE = 1ull << 0,

    /*
     * If set to 1 the device can respond to Memory Space accesses; otherwise,
     * the device's response is disabled.
     */

    __PCI_DEVCMDREG_MEMSPACE = 1ull << 1,

    /*
     * If set to 1 the device can behave as a bus master; otherwise, the device
     * can not generate PCI accesses.
     */

    __PCI_DEVCMDREG_BUS_MASTER = 1ull << 2,

    /*
     * If set to 1 the device can generate special cycles; otherwise, the device
     * can not generate special cycles.
     *
     * This bit was originally described in the [PCI]. Its functionality does
     * not apply to PCI Express and the bit must be hardwired to 0b.
     */

    __PCI_DEVCMDREG_SPECIAL_CYCLES = 1ull << 3,

    /*
     * If set to 1 the device can respond to the Memory-Write-Invalidate
     * command; otherwise, the device's response is disabled.
     *
     * - This bit was originally described in the [PCI] and the
     * [PCI-to-PCI-Bridge].
     * Its functionality does not apply to PCI Express and the bit must be
     * hardwired to 0b. For PCI Express to PCI/PCI-X Bridges, refer to the
     * [PCIe-to-PCI-PCI-X-Bridge] for requirements for this register.
     */

    __PCI_DEVCMDREG_MEMWRITE_INVALIDATE = 1ull << 4,

    /*
     * If set to 1 the device does not respond to palette register writes and
     * will snoop the data; otherwise, the device will treat palette write
     * accesses like all other accesses.
     *
     * This bit was originally described in the [PCI] and the
     * [PCI-to-PCI-Bridge]. Its functionality does not apply to PCI Express and
     * the bit must be hardwired to 0b.
     */

    __PCI_DEVCMDREG_VGA_PALETTE_SNOOP = 1ull << 5,

    /*
     * If set to 1 the device can respond to the Parity Error Response command;
     * otherwise, the device's response is disabled.
     */

    __PCI_DEVCMDREG_PAITY_ERR_RESP = 1ull << 6,

    /*
     * If set to 1 the device can respond to the SERR# Response command;
     * otherwise, the device's response is disabled.
     */

    __PCI_DEVCMDREG_SERR_RESP = 1ull << 8,

    /*
     * If set to 1 the device can respond to the Fast Back-to-Back Capable
     * command; otherwise, the device's response is disabled.
     */

    __PCI_DEVCMDREG_FAST_BACKTOBACK = 1ull << 9,

    /*
     * If set to 1 the assertion of the devices INTx# signal is disabled;
     * otherwise, assertion of the signal is enabled.
     */

    __PCI_DEVCMDREG_INT_DISABLE = 1ull << 10,
};

enum pci_spec_device_status_flags {
    __PCI_DEVSTATUS_INTERRUPT = 1ull << 3,
    __PCI_DEVSTATUS_CAPABILITIES = 1ull << 4,
    __PCI_DEVSTATUS_66MHZ_SUPPORT = 1ull << 5,
    __PCI_DEVSTATUS_UDF_SUPPORT = 1ull << 6,
    __PCI_DEVSTATUS_FAST_BACKTOBACK = 1ull << 7,
    __PCI_DEVSTATUS_DATA_PARITY_DETECT = 1ull << 8,
    __PCI_DEVSTATUS_DEVSEL_TIMING = 0b11ull << 9,
    __PCI_DEVSTATUS_SIGNAL_TARGET_ABORT = 1ull << 11,
    __PCI_DEVSTATUS_RECEIVED_TARGET_ABORT = 1ull << 12,
    __PCI_DEVSTATUS_RECEIVED_MASTER_ABORT = 1ull << 13,
    __PCI_DEVSTATUS_SIGNALED_SYSERR = 1ull << 14,
    __PCI_DEVSTATUS_PARITY_ERR_DETECTED = 1ull << 15,
};

enum pci_spec_devbar_memspace_kind {
    PCI_DEVBAR_MEMSPACE_32B = 0,
    PCI_DEVBAR_MEMSPACE_64B = 2,
};

enum pci_spec_device_bar_shifts {
    __PCI_DEVBAR_MEMKIND_SHIFT = 1,
    __PCI_DEVBAR_PREFETCHABLE_SHIFT = 3
};

enum pci_spec_device_bar_flags {
    __PCI_DEVBAR_IO = 1ull << 0,
    __PCI_DEVBAR_MEMKIND_MASK = 0b11 << __PCI_DEVBAR_MEMKIND_SHIFT,
    __PCI_DEVBAR_PREFETCHABLE = 1ull << __PCI_DEVBAR_PREFETCHABLE_SHIFT,
};

struct pci_spec_device_info_base {
    uint16_t vendor_id;
    uint16_t device_id;

    uint16_t command;
    uint16_t status;

    // prog_if = "Programming Interface"
    uint8_t revision_id;
    uint8_t prog_if;
    uint8_t subclass;
    uint8_t class_code;

    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_kind;
    uint8_t bist;
} __packed;

// Type 0
struct pci_spec_device_info {
    struct pci_spec_device_info_base base;

    volatile uint32_t bar_list[6];
    uint32_t cardbus_cis_pointer;

    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom_base_address;

    uint8_t capabilities_offset; // Offset from start of this structure
    uint8_t reserved[7];

    uint8_t interrupt_line;
    uint8_t interrupt_pin;

    const uint8_t min_grant;
    const uint8_t max_latency; // In units of 1/4 milliseconds

    char data[4032];
} __packed;

// Type 1
struct pci_spec_pci_to_pci_bridge_device_info {
    struct pci_spec_device_info_base base;

    volatile uint32_t bar_list[2];
    uint8_t primary_bus_number;
    uint8_t secondary_bus_number;
    uint8_t subordinate_bus_number;
    uint8_t secondary_latency_timer;
    uint8_t io_base;
    uint8_t io_limit;
    uint16_t secondary_status;
    uint16_t memory_base;
    uint16_t memory_limit;
    uint16_t prefetchable_memory_base;
    uint16_t prefetchable_memory_limit;
    uint16_t prefetchable_base_upper32;
    uint16_t prefetchable_limit_upper32;
    uint16_t io_base_upper16;
    uint16_t io_limit_upper16;
    uint8_t capabilities_offset;
    uint8_t reserved[3];
    uint32_t expansion_rom_base_address;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint16_t bridge_control;

    char data[4036];
} __packed;

struct pci_spec_pci_to_cardbus_bridge_device_info {
    struct pci_spec_device_info_base base;

    union {
        uint32_t cardbus_socket;
        uint32_t exca_base_address;
    };

    uint8_t capabilities_list_offset;
    uint8_t reserved[1];
    uint16_t secondary_status;
    uint16_t pci_bus_number;
    uint32_t cardbus_bus_number;
    uint8_t subordinate_bus_number;
    uint8_t cardbus_latency_timer;
    uint32_t memory_base0;
    uint32_t memory_limit0;
    uint32_t memory_base1;
    uint32_t memory_limit1;
    uint32_t io_base0;
    uint32_t io_limit0;
    uint32_t io_base1;
    uint32_t io_limit1;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint16_t bridge_control;
    uint16_t subsystem_device_id;
    uint16_t subsystem_vendor_id;
    uint32_t legacy_base_address;
} __packed;

enum pci_spec_cap_id {
    PCI_SPEC_CAP_ID_NULL,
    PCI_SPEC_CAP_ID_POWER_MANAGEMENT,
    PCI_SPEC_CAP_ID_AGP,
    PCI_SPEC_CAP_ID_VPD,
    PCI_SPEC_CAP_ID_SLOT_ID,
    PCI_SPEC_CAP_ID_MSI,
    PCI_SPEC_CAP_ID_COMPACT_PCI_HOT_SWAP,
    PCI_SPEC_CAP_ID_PCI_X,
    PCI_SPEC_CAP_ID_HYPER_TRANSPORT,
    PCI_SPEC_CAP_ID_VENDOR_SPECIFIC,
    PCI_SPEC_CAP_ID_DEBUG_PORT,
    PCI_SPEC_CAP_ID_COMPACT_PCI_CENTRAL_RSRC_CNTRL,
    PCI_SPEC_CAP_ID_PCI_HOTPLUG,
    PCI_SPEC_CAP_ID_PCI_BRIDGE_SYS_VENDOR_ID,
    PCI_SPEC_CAP_ID_AGP_8X,
    PCI_SPEC_CAP_ID_SECURE_DEVICE,
    PCI_SPEC_CAP_ID_PCI_EXPRESS,
    PCI_SPEC_CAP_ID_MSI_X,
    PCI_SPEC_CAP_ID_SATA,
    PCI_SPEC_CAP_ID_ADV_FEATURES,
    PCI_SPEC_CAP_ID_ENHANCED_ALLOCS,
    PCI_SPEC_CAP_ID_FLAT_PORTAL_BRIDGE,
};

struct pci_spec_capability {
    uint8_t id;
    uint8_t offset_to_next;
} __packed;

enum pci_spec_cap_msi_control_flags {
    __PCI_CAPMSI_CTRL_ENABLE = 1 << 0,
    __PCI_CAPMSI_CTRL_MULTIMSG_CAPABLE = 0b111 << 1,
    __PCI_CAPMSI_CTRL_MULTIMSG_ENABLE = 0b111 << 4,

    __PCI_CAPMSI_CTRL_64BIT_CAPABLE  = 1 << 7,
    __PCI_CAPMSI_CTRL_PER_VECTOR_MASK = 1 << 8
};

enum pci_spec_cap_msix_control_flags {
    __PCI_CAP_MSIX_TABLE_SIZE_MASK = (1ull << 10) - 1,
    __PCI_CAP_MSIX_CTRL_ENABLE = 1 << 15
};

struct pci_spec_cap_msix_table_entry {
    uint32_t msg_address_lower32;
    uint32_t msg_address_upper32;
    uint32_t data;
    uint32_t control;
} __packed;

struct pci_spec_cap_msi {
    struct pci_spec_capability base;

    uint16_t msg_control;
    uint32_t msg_address;

    union {
        struct {
            uint32_t msg_address_upper; // Only if 64-bit capable
            uint16_t msg_data;
            uint32_t mask_bits;
            uint32_t pending_bits;
        } __packed bits64;
        struct {
            uint16_t msg_data;
            uint32_t mask_bits;
            uint32_t pending_bits;
        } __packed bits32;
    };

} __packed;

enum pci_spec_bar_table_offset_flags {
    __PCI_BARSPEC_TABLE_OFFSET_BIR = 0b111
};

struct pci_spec_cap_msix {
    struct pci_spec_capability base;
    uint16_t msg_control;
    uint32_t msg_address_upper;
    uint32_t table_offset; // Lower 3 Bits are the BIR
} __packed;

enum pci_spec_cap_pcie_dev_port_kind {
    PCI_SPEC_CAP_PCIE_DEVPORT_PCIE_ENDPOINT,
    PCI_SPEC_CAP_PCIE_DEVPORT_LEGACY_PCIE_ENDPOINT,
    PCI_SPEC_CAP_PCIE_DEVPORT_ROOT_PORT_PCIE_ROOT_COMPLEX = 4,
    PCI_SPEC_CAP_PCIE_DEVPORT_UPSTREAM_PORT_PCIE_SWITCH,
    PCI_SPEC_CAP_PCIE_DEVPORT_DOWNSTREAM_PORT_PCIE_SWITCH,
    PCI_SPEC_CAP_PCIE_DEVPORT_PCIE_TO_PCI_PCIX_BRIDGE,
    PCI_SPEC_CAP_PCIE_DEVPORT_ROOT_COMPLEX_INTEGRATED_ENDPOINT,
    PCI_SPEC_CAP_PCIE_DEVPORT_ROOT_COMPLEX_EVENT_COLLECTOR,
};

enum pci_spec_cap_pcie_capability_shifts {
    __PCIE_SPEC_CAP_PCIE_CAP_VERSION_SHIFT,
    __PCIE_SPEC_CAP_PCIE_CAP_DEV_PORT_SHIFT = 4,
    __PCIE_SPEC_CAP_PCIE_CAP_INT_MSG_NUM_SHIFT = 9
};

enum pci_spec_cap_pcie_capability_masks {
    __PCIE_SPEC_CAP_PCIE_CAP_VERSION_MASK = 0b1111,
    __PCIE_SPEC_CAP_PCIE_CAP_DEV_PORT_TYPE_MASK =
        0b1111 << __PCIE_SPEC_CAP_PCIE_CAP_DEV_PORT_SHIFT,
    __PCIE_SPEC_CAP_PCIE_CAP_INT_MSG_NUM_MASK =
        0b1111 << __PCIE_SPEC_CAP_PCIE_CAP_DEV_PORT_SHIFT
};

enum pcie_spec_cap_pcie_capability_flags {
    /*
     * When Set, this bit indicates that the PCI Express Link associated with
     * this Port is connected to a slot (as compared to being connected to an
     * integrated component or being disabled)
     */

    __PCIE_SPEC_CAP_PCIE_CAP_SLOT_IMPLED = 1 << 8,

    /*
     * When Set, this bit indicates that the PCI Express Switch or Root Port
     * supports routing of Trusted Configuration Requests. Switches that support
     * routing of Trusted Configuration Requests must Set this bit on both
     * upstream and downstream ports. Root Ports that support routing of Trusted
     * Configuration Requests must also Set this bit.
     *
     * It is permitted for a Root Port to set this bit even if the TCAM is not
     * available on the platform, in which case the Root Complex as a whole does
     * not support generation of Trusted Configuration Requests.
     *
     * Implementing the TCS Routing Supported Capability bit in standard PCI
     * Express Configuration Space allows software to determine whether a given
     * Trusted Device’s TCS may be accessed, based on whether or not the Root
     * Port and all intervening Switches properly route Trusted Configuration
     * Requests.
     *
     * This mechanism is useful, for example, to facilitate troubleshooting in
     * systems where the user has plugged a Trusted Device into a slot that is
     * downstream of a Switch that doesn’t support TCS routing. Software could
     * detect this condition, and assist the user in finding a suitable slot in
     * which to plug in their Trusted Device.
     */

    __PCIE_SPEC_CAP_PCIE_CAP_TCS_ROUTING_SUPPORTED = 1 << 14
};

enum pci_spec_cap_pcie_devcap_max_payload_size {
    PCI_SPEC_CAP_PCIE_DEVCAP_MAX_PAYLOAD_128B,
    PCI_SPEC_CAP_PCIE_DEVCAP_MAX_PAYLOAD_256B,
    PCI_SPEC_CAP_PCIE_DEVCAP_MAX_PAYLOAD_512B,

    PCI_SPEC_CAP_PCIE_DEVCAP_MAX_PAYLOAD_1KiB,
    PCI_SPEC_CAP_PCIE_DEVCAP_MAX_PAYLOAD_2KiB,
    PCI_SPEC_CAP_PCIE_DEVCAP_MAX_PAYLOAD_4KiB,
};

enum pci_spec_cap_pcie_devcap_phantom_func_kind {
    PCI_SPEC_CAP_PCIE_DEVCAP_PHANTOM_FUNC_NONE,

    /*
     * The most significant bit of the Function number in Requester ID is used
     * for Phantom Functions; a multi-Function device is permitted to implement
     * Functions 0-3.
     * Functions 0, 1, 2, and 3 are permitted to use
     * Function Numbers 4, 5, 6, and 7 respectively as Phantom Functions.
     */

    PCI_SPEC_CAP_PCIE_DEVCAP_PHANTOM_FUNC_MSB,

    /*
     * The two most significant bits of Function Number in Requester ID are used
     * for Phantom Functions; a multi-Function device is permitted to implement
     * Functions 0-1. Function 0 is permitted to use
     * Function Numbers 2, 4, and 6 for Phantom
     * Functions. Function 1 is permitted to use Function
     * Numbers 3, 5, and 7 as Phantom Functions.
     */

    PCI_SPEC_CAP_PCIE_DEVCAP_PHANTOM_FUNC_2_MSB,

    /*
     * All 3 bits of Function Number in Requester ID used for Phantom Functions.
     * The device must have a single Function 0 that is permitted to use all
     * other Function Numbers as Phantom Functions.
     */

    PCI_SPEC_CAP_PCIE_DEVCAP_PHANTOM_FUNC_3,
};

enum pci_spec_cap_pcie_devcap_l0_acceptable_latency {
    PCI_SPEC_CAP_PCIE_DEVCAP_L0_ACCEPTABLE_LATENCY_64NS_MAX,
    PCI_SPEC_CAP_PCIE_DEVCAP_L0_ACCEPTABLE_LATENCY_128NS_MAX,
    PCI_SPEC_CAP_PCIE_DEVCAP_L0_ACCEPTABLE_LATENCY_256NS_MAX,
    PCI_SPEC_CAP_PCIE_DEVCAP_L0_ACCEPTABLE_LATENCY_512NS_MAX,
    PCI_SPEC_CAP_PCIE_DEVCAP_L0_ACCEPTABLE_LATENCY_1US_MAX,
    PCI_SPEC_CAP_PCIE_DEVCAP_L0_ACCEPTABLE_LATENCY_2US_MAX,
    PCI_SPEC_CAP_PCIE_DEVCAP_L0_ACCEPTABLE_LATENCY_4US_MAX,
    PCI_SPEC_CAP_PCIE_DEVCAP_L0_ACCEPTABLE_LATENCY_NO_LIMIT,
};

enum pci_spec_cap_pcie_devcap_l1_acceptable_latency {
    PCI_SPEC_CAP_PCIE_DEVCAP_L1_ACCEPTABLE_LATENCY_1US_MAX,
    PCI_SPEC_CAP_PCIE_DEVCAP_L1_ACCEPTABLE_LATENCY_2US_MAX,
    PCI_SPEC_CAP_PCIE_DEVCAP_L1_ACCEPTABLE_LATENCY_4US_MAX,
    PCI_SPEC_CAP_PCIE_DEVCAP_L1_ACCEPTABLE_LATENCY_8US_MAX,
    PCI_SPEC_CAP_PCIE_DEVCAP_L1_ACCEPTABLE_LATENCY_16US_MAX,
    PCI_SPEC_CAP_PCIE_DEVCAP_L1_ACCEPTABLE_LATENCY_32US_MAX,
    PCI_SPEC_CAP_PCIE_DEVCAP_L1_ACCEPTABLE_LATENCY_64US_MAX,
    PCI_SPEC_CAP_PCIE_DEVCAP_L1_ACCEPTABLE_LATENCY_NO_LIMIT,
};

enum pci_spec_cap_pcie_devcap_shifts {
    __PCI_SPEC_CAP_PCIE_DEVCAP_MAX_PAYLOAD_SIZE_SHIFT,
    __PCI_SPEC_CAP_PCIE_DEVCAP_PHANTOM_FUNC_SHIFT = 3,
    __PCI_SPEC_CAP_PCIE_DEVCAP_ENDPOINT_L0_ACCEPTABLE_LATENCY_SHIFT = 6,
    __PCI_SPEC_CAP_PCIE_DEVCAP_ENDPOINT_L1_ACCEPTABLE_LATENCY_SHIFT = 9,
    __PCI_SPEC_CAP_PCIE_DEVCAP_CAPTURED_SLOT_PWR_LIMIT_SHIFT = 18
};

enum pci_spec_cap_pcie_devcap_masks {
    __PCI_SPEC_CAP_PCIE_DEVCAP_MAX_PAYLOAD_SIZE_MASK = 0b111,
    __PCI_SPEC_CAP_PCIE_DEVCAP_PHANTOM_FUNC_MASK =
        0b11 << __PCI_SPEC_CAP_PCIE_DEVCAP_PHANTOM_FUNC_SHIFT,
    __PCI_SPEC_CAP_PCIE_DEVCAP_ENDPOINT_L0_ACCEPTABLE_LATENCY =
        0b111ull <<
            __PCI_SPEC_CAP_PCIE_DEVCAP_ENDPOINT_L0_ACCEPTABLE_LATENCY_SHIFT,
    __PCI_SPEC_CAP_PCIE_DEVCAP_ENDPOINT_L1_ACCEPTABLE_LATENCY =
        0b111ull <<
            __PCI_SPEC_CAP_PCIE_DEVCAP_ENDPOINT_L1_ACCEPTABLE_LATENCY_SHIFT,
    __PCI_SPEC_CAP_PCIE_DEVCAP_CAPTURED_SLOT_PWR_LIMIT =
        0xff << __PCI_SPEC_CAP_PCIE_DEVCAP_CAPTURED_SLOT_PWR_LIMIT_SHIFT,
};

enum pci_spec_cap_pcie_devcap_flags {
    // 8-bit Tag field supported if set, otherwise 5-bit Tag field supported.
    __PCI_SPEC_CAP_PCIE_DEVCAP_EXT_TAG_SUPPORTED = 1 << 5,
    __PCI_SPEC_CAP_PCIE_DEVCAP_FUNC_LVL_RESET_CAP = 1 << 28,
};

struct pci_spec_cap_pcie {
    struct pci_spec_capability base;

    uint16_t capabilities;
    uint32_t device_capabilities;
    uint32_t device_control;
} __packed;