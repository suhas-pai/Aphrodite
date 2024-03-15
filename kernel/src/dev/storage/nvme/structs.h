/*
 * kernel/src/nvme/structs.h
 * © suhas pai
 */

#pragma once
#include "lib/macros.h"

enum nvme_cmd_set_support {
    NVME_CMD_SET_SUPPORT_NONE = 7,
    NVME_CMD_SET_SUPPORT_ONE_OR_MORE = 6
};

enum nvme_capability_shifts {
    NVME_CAP_DOORBELL_STRIDE_SHIFT = 32,
    NVME_CAP_CMD_SETS_SUPPORT_SHIFT = 37,
    NVME_CAP_CNTLR_POWER_SUPPORT_SHIFT = 46,
    NVME_CAP_MEM_PAGE_SIZE_MIN_SHIFT = 48,
    NVME_CAP_MEM_PAGE_SIZE_MAX_SHIFT = 52,
    NVME_CAP_CNTLR_READY_MODES_SUPPORTED_SHIFT = 59
};

enum nvme_capabilities {
    __NVME_CAP_MAX_QUEUE_ENTRIES = 0xFF,
    __NVME_CAP_CONTIG_QUEUE_SUPPORTED = 1 << 16,
    __NVME_CAP_ARBITR_MECH_SUPPORTED = 1 << 17,
    __NVME_CAP_TIMEOUT_500MS = 0xFF << 24,

    // Specified as (2 ^ (2 + value)) in bytes
    __NVME_CAP_DOORBELL_STRIDE = 0xFull << NVME_CAP_DOORBELL_STRIDE_SHIFT,
    __NVME_CAP_NVM_SUBSYSTEM_RESET_SUPPORTED = 1ull << 36,
    __NVME_CAP_CMD_SETS_SUPPORT = 0xFFull << NVME_CAP_CMD_SETS_SUPPORT_SHIFT,
    __NVME_CAP_BOOT_PARTITION_SUPPORTED = 1ull << 45,
    __NVME_CAP_CNTLR_POWER_SUPPORT =
        0b11ull << NVME_CAP_CNTLR_POWER_SUPPORT_SHIFT,

    // The memory page size is (2 ^ (12 + value))
    __NVME_CAP_MEM_PAGE_SIZE_MIN_4KiB =
        0xFull << NVME_CAP_MEM_PAGE_SIZE_MIN_SHIFT,
    // The memory page size is (2 ^ (12 + value))
    __NVME_CAP_MEM_PAGE_SIZE_MAX = 0xFull << NVME_CAP_MEM_PAGE_SIZE_MAX_SHIFT,

    __NVME_CAP_PERSISTENT_MEM_REGION_SUPPORTED = 1ull << 56,
    __NVME_CAP_CNTLR_MEM_BUFFER_SUPPORTED = 1ull << 57,
    __NVME_CAP_NVM_SUBSYSTEM_SHUTDOWN_SUPPPORTED = 1ull << 58,
    __NVME_CAP_CNTLR_READY_MODES_SUPPORTED =
        0b11ull << NVME_CAP_CNTLR_READY_MODES_SUPPORTED_SHIFT
};

enum nvme_version_shifts {
    NVME_VERSION_MINOR_SHIFT = 8,
    NVME_VERSION_MAJOR_SHIFT = 16,
};

enum nvme_version_masks {
    __NVME_VERSION_TERTIARY = 0xFF,
    __NVME_VERSION_MINOR = 0xFF << NVME_VERSION_MINOR_SHIFT,
    __NVME_VERSION_MAJOR = 0xFFFFull << NVME_VERSION_MAJOR_SHIFT,
};

#define NVME_VERSION_FMT "%" PRIu16 ".%" PRIu16 ".%" PRIu16
#define NVME_VERSION_FMT_ARGS(version) \
    (version & __NVME_VERSION_MAJOR) >> NVME_VERSION_MAJOR_SHIFT, \
    (version & __NVME_VERSION_MINOR) >> NVME_VERSION_MINOR_SHIFT, \
    version & __NVME_VERSION_TERTIARY

enum nvme_config_arbitration_method {
    NVME_CONFIG_ARB_METHOD_ROUND_ROBIN,
    // Weighted Round Robin with Urgent Priority Class
    NVME_CONFIG_ARB_METHOD_WEIGHTED_ROUND_ROBIN,
    NVME_CONFIG_ARB_METHOD_VENDOR_SPECIFIC = 0b111,
};

enum nvme_config_shutdown_notif {
    NVME_CONFIG_SHUTDOWN_NOTIF_NONE,
    NVME_CONFIG_SHUTDOWN_NOTIF_NORMAL,
    NVME_CONFIG_SHUTDOWN_NOTIF_ABRUPT,
};

enum nvme_config_css {
    NVME_CONFIG_CSS
};

enum nvme_config_shifts {
    NVME_CONFIG_CMD_SET_SELECTED_SHIFT = 4,
    NVME_CONFIG_MEM_PAGE_SIZE_SHIFT = 7,
    NVME_CONFIG_ARB_METHOD_SHIFT = 11,
    NVME_CONFIG_SHUTDOWN_NOTIF_SHIFT = 14,
    NVME_CONFIG_IO_SUBMIT_QUEUE_ENTRY_SIZE_SHIFT = 16,
    NVME_CONFIG_IO_COMPL_QUEUE_ENTRY_SIZE_SHIFT = 20
};

enum nvme_config_flags {
    __NVME_CONFIG_ENABLE = 1 << 0,

    /*
     * This field specifies the I/O Command Set or Sets that are selected. This
     * field shall only be changed when the controller is disabled (i.e., CC.EN
     * is cleared to ‘0’). The I/O Command Set or Sets that are selected shall
     * be used for all I/O Submission Queues.
     */
    __NVME_CONFIG_CMD_SET_SELECTED = 0b111 << 4,

    //  The memory page size is (2 ^ (12 + value))
    __NVME_CONFIG_MEM_PAGE_SIZE = 0xF << NVME_CONFIG_MEM_PAGE_SIZE_SHIFT,
    __NVME_CONFIG_ARB_METHOD = 0b111 << NVME_CONFIG_ARB_METHOD_SHIFT,
    __NVME_CONFIG_SHUTDOWN_NOTIF =
        0b11 << NVME_CONFIG_SHUTDOWN_NOTIF_SHIFT,
    __NVME_CONFIG_IO_SUBMIT_QUEUE_ENTRY_SIZE =
        0xF << NVME_CONFIG_IO_SUBMIT_QUEUE_ENTRY_SIZE_SHIFT,
    __NVME_CONFIG_IO_COMPL_QUEUE_ENTRY_SIZE =
        0xF << NVME_CONFIG_IO_COMPL_QUEUE_ENTRY_SIZE_SHIFT,

    __NVME_CONFIG_CNTLR_READY_INDEP_OF_MEDIA_ENABLE = 1 << 24
};

enum nvme_controller_status_shutdown {
    NVME_CNTLR_STATUS_SHUTDOWN_NORMAL,
    NVME_CNTLR_STATUS_SHUTDOWN_OCCURRING,
    NVME_CNTLR_STATUS_SHUTDOWN_COMPLETE,
};

enum nvme_controller_status_shifts {
    NVME_CNTLR_STATUS_SHUTDOWN_SHIFT = 2,
};

enum nvme_controller_status_flags {
    __NVME_CNTLR_STATUS_READY = 1 << 0,
    __NVME_CNTLR_STATUS_FATAL = 1 << 1,
    __NVME_CNTLR_STATUS_SHUTDOWN = 0b11 << NVME_CNTLR_STATUS_SHUTDOWN_SHIFT,

    __NVME_CNTLR_STATUS_NVM_SUBSYS_RESET = 1 << 4,
    __NVME_CNTLR_STATUS_PROCESS_PAUSED = 1 << 5,

    /*
     * If this bit is set to ‘1’, then CSTS.SHST is reporting the state of an
     * NVM Subsystem Shutdown. If this bit is cleared to ‘0’, then CSTS.SHST is
     * reporting the state of a controller shutdown.
     */
    __NVME_CNTLR_STATUS_SHUTDOWN_TYPE = 1 << 5,
};

#define NVME_SUBSYS_RESET_VALUE 0x4E564D65

enum nvme_admin_queue_attr_shifts {
    NVME_ADMIN_QUEUE_ATTR_SUBMIT_QUEUE_SIZE_SHIFT = 16,
};

enum nvme_admin_queue_attr_flags {
    /*
     * The minimum size of the Admin Completion Queue is two entries. The
     * maximum size of the Admin Completion Queue is 4,096 entries. This is a
     * 0’s based value.
     */
    __NVME_ADMIN_QUEUE_ATTR_SUBMIT_QUEUE_SIZE = mask_for_n_bits(11),
    /*
     * The minimum size of the Admin Completion Queue is two entries. The
     * maximum size of the Admin Completion Queue is 4,096 entries. This is a
     * 0’s based value.
     */
    __NVME_ADMIN_QUEUE_ATTR_COMPL_QUEUE_SIZE = mask_for_n_bits(12) << 16,
};

enum nvme_cntlr_mem_buffer_loc_shifts {
    NVME_CNTLR_CMB_OFFSET_SHIFT = 12,
};

enum nvme_cntlr_mem_buffer_loc_flags {
    __NVME_CNTLR_CMB_LOC_MEM_BUFFER_LOC_BIR = 0b111,
    __NVME_CNTLR_CMB_LOC_MIXED_MEM_SUPPORT = 1 << 3,
    __NVME_CNTLR_CMB_LOC_PHYSICAL_MEM_DISCONTIG_SUPPORT = 1 << 4,
    __NVME_CNTLR_CMB_LOC_DATA_PTR_MIXED_LOC_SUPPORT = 1 << 5,
    __NVME_CNTLR_CMB_LOC_DATA_PTR_AND_CMD_INDEP_LOC_SUPPORT = 1 << 6,
    __NVME_CNTLR_CMB_LOC_METADATA_MIXED_LOC_SUPPORT = 1 << 7,
    __NVME_CNTLR_CMB_LOC_QUEUE_DWORD_ALIGNMENT = 1 << 8,

    // Indicates the offset of the Controller Memory Buffer in multiples of the
    // Size Unit specified in CMBSZ.
    __NVME_CNTLR_CMB_LOC_OFFSET =
        mask_for_n_bits(20) << NVME_CNTLR_CMB_OFFSET_SHIFT
};

enum nvme_cntlr_cmb_size_units_gran {
    NVME_CNTLR_CMB_SIZE_UNITS_GRAN_4KiB,
    NVME_CNTLR_CMB_SIZE_UNITS_GRAN_64KiB,
    NVME_CNTLR_CMB_SIZE_UNITS_GRAN_1MiB,
    NVME_CNTLR_CMB_SIZE_UNITS_GRAN_16MiB,
    NVME_CNTLR_CMB_SIZE_UNITS_GRAN_256MiB,
    NVME_CNTLR_CMB_SIZE_UNITS_GRAN_4GiB,
    NVME_CNTLR_CMB_SIZE_UNITS_GRAN_64GiB,
};

enum nvme_cntlr_mem_buffer_size_shifts {
    NVME_CNTLR_CMB_SIZE_UNITS_GRAN_SHIFT = 4,
    NVME_CNTLR_CMB_SIZE_SHIFT = 12,
};

enum nvme_cntlr_mem_buffer_size_flags {
    __NVME_CNTLR_CMB_SIZE_SUBMIT_QUEUE_SUPPORT = 1 << 0,
    __NVME_CNTLR_CMB_SIZE_COMPL_QUEUE_SUPPORT = 1 << 1,
    __NVME_CNTLR_CMB_SIZE_PRP_SGL_LIST_SUPPORT = 1 << 2,
    __NVME_CNTLR_CMB_SIZE_READ_DATA_SUPPORT = 1 << 3,
    __NVME_CNTLR_CMB_SIZE_WRITE_DATA_SUPPORT = 1 << 4,
    __NVME_CNTLR_CMB_SIZE_UNITS_GRAN =
        0xF << NVME_CNTLR_CMB_SIZE_UNITS_GRAN_SHIFT,
    __NVME_CNTLR_CMB_SIZE =
        mask_for_n_bits(20) << NVME_CNTLR_CMB_SIZE_SHIFT,
};

enum nvme_cntlr_boot_partition_info_read_status {
    NVME_CNTLR_BOOT_PARTITION_INFO_READ_STATUS_NONE,
    NVME_CNTLR_BOOT_PARTITION_INFO_READ_STATUS_IN_PROGRESS,
    NVME_CNTLR_BOOT_PARTITION_INFO_READ_STATUS_SUCCESS,
    NVME_CNTLR_BOOT_PARTITION_INFO_READ_STATUS_ERROR,
};

enum nvme_cntlr_boot_partition_info_shifts {
    NVME_CNTLR_BOOT_PARTITION_INFO_BOOT_READ_STATUS_SHIFT = 15,
};

enum nvme_cntlr_boot_partition_info_flags {
    __NVME_CNTLR_BOOT_PARTITION_INFO_SIZE_128KiB = mask_for_n_bits(14),
    __NVME_CNTLR_BOOT_PARTITION_INFO_READ_STATUS =
        0b11 << NVME_CNTLR_BOOT_PARTITION_INFO_BOOT_READ_STATUS_SHIFT,

    __NVME_CNTLR_BOOT_PARTITION_INFO_ACTIVE_ID = 1ull << 31
};

enum nvme_cntlr_boot_partition_read_select_shifts {
    NVME_CNTLR_BOOT_PARTITION_RD_SELECT_OFFSET_4KiB_SHIFT = 10,
};

enum nvme_cntlr_boot_partition_read_select_flags {
    __NVME_CNTLR_BOOT_PARTITION_RD_SELECT_SIZE_4KiB = mask_for_n_bits(10),
    __NVME_CNTLR_BOOT_PARTITION_RD_SELECT_OFFSET_4KiB =
        mask_for_n_bits(10) <<
            NVME_CNTLR_BOOT_PARTITION_RD_SELECT_OFFSET_4KiB_SHIFT,

    __NVME_CNTLR_BOOT_PARTITION_RD_SELECT_ID = 1ull << 31,
};

enum nvme_cntlr_cmb_control_flags {
    __NVME_CNTLR_CMB_CONTROL_CAP_REG_ENABLED = 1 << 0,
    __NVME_CNTLR_CMB_CONTROL_MEM_SPACE_ENABLED = 1 << 1,
    __NVME_CNTLR_CMB_CONTROL_BASE_ADDR = mask_for_n_bits(52) << 12,
};

enum nvme_cntlr_cmb_status_flags {
    __NVME_CNTLR_CMB_STATUS_BASE_ADDR_INVALID = 1 << 0,
};

struct nvme_doorbell_regs {
    volatile uint32_t submit;
    volatile uint32_t completion;
};

struct nvme_registers {
    volatile uint64_t capabilities;
    volatile uint32_t version;
    volatile uint32_t intr_vector_mask_set;
    volatile uint32_t intr_vector_mask_clear;
    volatile uint32_t config;
    volatile uint32_t reserved_1;
    volatile uint32_t status;
    volatile uint32_t subsys_reset;
    volatile uint32_t admin_queue_attributes;

    // Must be 4k aligned
    volatile uint64_t admin_submit_queue_base_addr;
    // Must be 4k aligned
    volatile uint64_t admin_completion_queue_base_addr;

    volatile uint32_t cmb_loc;
    volatile uint32_t cmb_size;

    volatile uint32_t boot_partition_info;
    volatile uint32_t boot_partition_read_select;

    // Must be 4k aligned
    volatile uint64_t boot_partition_mem_buf_base_addr;

    volatile uint32_t cmb_control;
    volatile uint64_t cmb_status;
    volatile uint32_t cmb_elasticity_buffer_size;
    volatile uint32_t cmb_sustain_write_throughput;

    volatile uint32_t nvm_subsystem_shutdown;
    volatile uint32_t cntlr_ready_timeouts;

    volatile const uint8_t reserved[3476];

    volatile uint32_t pmr_capabilities;
    volatile uint32_t pmr_control;
    volatile uint32_t pmr_status;
    volatile uint32_t pmr_elasticity_bufsize;
    volatile uint32_t pmr_sustained_write_throughput;

    volatile uint32_t pmr_sustained_memspace_ctrl_lower32;
    volatile uint32_t pmr_sustained_memspace_ctrl_upper32;

    volatile const uint8_t reserved_2[484];
    volatile const uint8_t doorbell[];
} __packed;

enum nvme_identify_cns {
    NVME_IDENTIFY_CNS_NAMESPACE,
    NVME_IDENTIFY_CNS_CONTROLLER,
    NVME_IDENTIFY_CNS_ACTIVE_NSID_LIST,
    NVME_IDENTIFY_CNS_NSID_DESC_LIST,
};

enum nvme_controller_kind {
    NVME_CONTROLLER_KIND_NOT_REPORTED,
    NVME_CONTROLLER_KIND_IO_CONTROLLER,
    NVME_CONTROLLER_KIND_DISCOVERY_CONTROLLER,
    NVME_CONTROLLER_KIND_ADMINISTRATIVE_CONTROLLER,
};

enum nvme_admin_cmd_support_flags {
    __NVME_ADMIN_CMD_SUPPORTS_SECURITY_SEND_RECV = 1 << 0,
    __NVME_ADMIN_CMD_SUPPORTS_FORMAT_NVM = 1 << 1,
    __NVME_ADMIN_CMD_SUPPORTS_FIRMWARE_COMMIT_IMAGE_DL = 1 << 2,
    __NVME_ADMIN_CMD_SUPPORTS_NAMESPACE_MGMT = 1 << 3,
    __NVME_ADMIN_CMD_SUPPORTS_DEV_SELF_TEST = 1 << 4,
    __NVME_ADMIN_CMD_SUPPORTS_DIRECTIVES = 1 << 5,
    __NVME_ADMIN_CMD_SUPPORTS_NVME_MI_SEND_RECV = 1 << 6,
    __NVME_ADMIN_CMD_SUPPORTS_VIRT_MGMT = 1 << 7,
    __NVME_ADMIN_CMD_SUPPORTS_DOORBELL_CFG_CMD = 1 << 8,
    __NVME_ADMIN_CMD_SUPPORTS_GET_LBA_STATUS = 1 << 9,
    __NVME_ADMIN_CMD_SUPPORTS_CMD_FEAT_LOCKDOWN = 1 << 10,
};

enum nvme_cmdsupport_flags {
    __NVME_CMD_SUPPORTS_COMPARE = 1 << 0,
    __NVME_CMD_SUPPORTS_WRITE_UNCORRECTABLE = 1 << 1,
    __NVME_CMD_SUPPORTS_DATASET_MGMT = 1 << 2,
    __NVME_CMD_SUPPORTS_WRITE_ZEROES = 1 << 3,
    __NVME_CMD_SUPPORTS_SAVE_FIELD_NON_ZERO = 1 << 4,
    __NVME_CMD_SUPPORTS_RESERVATIONS = 1 << 5,
    __NVME_CMD_SUPPORTS_TIMESTAMP = 1 << 6,
    __NVME_CMD_SUPPORTS_VERIFY = 1 << 7,
    __NVME_CMD_SUPPORTS_COPY = 1 << 8,
};

struct nvme_controller_identity {
    uint16_t vendor_id;
    uint16_t subsystem_vendor_id;

    char serial_number[20];
    char model_number[40];
    char firmware_revision[8];

    uint8_t rec_arb_burst;
    uint8_t ieee_oui_id[3];
    uint8_t cmic;
    uint8_t max_data_transfer_shift;

    uint16_t controller_id;
    uint32_t version;
    uint32_t rtd3_resume_latency;
    uint32_t rtd3_entry_latency;

    uint32_t opt_async_events_support;
    uint32_t cntlr_attr;

    uint16_t read_recovery_levels_supported;
    const uint64_t reserved;

    uint8_t controller_kind;
    char fru_guid[16];

    uint16_t cmd_retry_delay_time1;
    uint16_t cmd_retry_delay_time2;
    uint16_t cmd_retry_delay_time3;

    const uint8_t reserved_2[120];

    uint8_t nvme_subsystem_report;
    uint8_t vpd_write_cycle_info;
    uint8_t mgmt_endpoint_capabilities;

    uint16_t opt_admin_cmd_support;

    uint8_t abort_cmd_limit;
    uint8_t async_event_req_limit;
    uint8_t firmware_updates;
    uint8_t log_page_attr;
    uint8_t error_log_page_entries;
    uint8_t num_power_states_support;
    uint8_t admin_vendor_specific_cmd_config;
    uint8_t autonomous_power_state_trans_attr;

    uint16_t warning_composite_temp_threshold;
    uint16_t critical_composite_temp_threshold;
    uint16_t max_time_for_fw_activation;

    // If this field is non-zero, then the Host Memory Buffer feature is
    // supported. If this field is cleared to 0h, then the Host Memory Buffer
    // feature is not supported.

    uint32_t host_membuf_pref_page_size_4kib;
    uint32_t host_membuf_min_page_size_4kib;

    uint64_t total_nvm_capacity_lower64;
    uint64_t total_nvm_capacity_upper64;

    uint64_t unalloc_nvm_capacity_lower64;
    uint64_t unalloc_nvm_capacity_upper64;

    uint32_t replay_protected_memblock_support;
    uint16_t ext_device_self_test_time;

    // Bit 0 if set to ‘1’, then the NVM subsystem supports only one device
    // self-test operation in progress at a time. If cleared to ‘0’, then the
    // NVM subsystem supports one device self-test operation per controller at a
    // time.

    uint8_t device_self_test_options;
    uint8_t fw_update_granularity;

    uint16_t keep_alive_support_100ms;
    uint16_t host_ctrl_therm_mgmt_attr;

    uint16_t min_therm_mgmt_temp;
    uint16_t max_therm_mgmt_temp;

    uint32_t sanitize_capabilities;

    uint32_t host_membuf_min_desc_entry_size;
    uint16_t host_membuf_max_desc_entries;
    uint16_t nvm_set_id_max;
    uint16_t endurance_group_id_min;

    uint8_t ana_transition_time;
    uint8_t asymmetric_namespace_access_cap;

    uint32_t ana_group_id_max;
    uint32_t ana_group_id_count;

    uint32_t persistent_event_log_size;

    uint16_t domain_id;
    const uint8_t reserved_3[10];

    uint16_t max_endurance_group_capacity;
    const uint8_t reserved_4[142];

    uint8_t submit_queue_entry_size;
    uint8_t completion_queue_entry_size;

    uint16_t max_outstanding_cmds;
    uint32_t namespace_count;
    uint16_t opt_nvm_cmd_support;
    uint16_t fused_oper_support;

    uint8_t format_nvm_attr;
    uint8_t volatile_write_cache;

    uint16_t atomic_write_unit_normal;
    uint16_t atomic_write_unit_power_fail;

    uint8_t io_cmdset_vendor_specifc_cmd_cfg;
    uint8_t namespace_write_protection_cap;

    uint16_t atomic_compare_write_unit;
    uint16_t copy_desc_formats_support;

    uint32_t sgl_support;
    uint32_t max_allowed_namespaces;

    uint64_t max_domain_space_attachments_lower64;
    uint64_t max_domain_space_attachments_upper64;

    uint32_t max_io_cntlr_namespace_attachments;
    const uint8_t reserved_5[204];

    uint8_t nvme_subsystem_qual_name[256];
} __packed;

struct nvme_lba {
    uint16_t metadata_size;

    uint8_t lba_data_size_shift;
    uint8_t relative_performance;
};

#define NVME_MAX_LBA_COUNT 64

struct nvme_namespace_identity {
    uint64_t size;
    uint64_t capacity;
    uint64_t utilization;

    uint8_t features;
    uint8_t formatted_lba_count;
    uint8_t formatted_lba_index;
    uint8_t metadata_capabilities;
    uint8_t data_protection_capabilities;
    uint8_t data_protection_settings;

    // Namespace multi-path I/O and Namespace sharing capabilities
    uint8_t nmic;
    uint8_t reservation_capabilities;
    uint8_t format_progress_indicator;
    uint8_t dealloc_lba_features;

    uint16_t namespace_atomic_write_unit_normal;
    uint16_t namespace_atomic_write_unit_power_fail;
    uint16_t namespace_cmp_and_write_unit;
    uint16_t namespace_boundary_size_normal;
    uint16_t namespace_boundary_offset;
    uint16_t namespace_boundary_size_power_fail;
    uint16_t namespace_optimal_io_boundary;

    uint64_t nvm_capability_lower64;
    uint64_t nvm_capability_upper64;

    uint16_t namespace_pref_write_gran;
    uint16_t namespace_pref_write_align;
    uint16_t namespace_pref_dealloc_gran;
    uint16_t namespace_pref_dealloc_align;

    uint16_t namespace_optim_write_size;
    uint16_t max_single_source_range_length;
    uint32_t max_copy_length;
    uint8_t min_source_range_count;

    uint32_t ana_group_identifier[3];
    uint8_t attributes;
    uint16_t nvm_set_id;
    uint16_t endurance_group_id;
    uint8_t guid[16];
    uint64_t iee_uid;

    struct nvme_lba lbaf[NVME_MAX_LBA_COUNT];
    uint8_t vendor_specific[3712];
};

enum nvme_command_opcode {
    NVME_CMD_OPCODE_FLUSH = 0,
    NVME_CMD_OPCODE_WRITE = 1,
    NVME_CMD_OPCODE_READ = 2,

    NVME_CMD_ADMIN_OPCODE_CREATE_SQ = 1,
    NVME_CMD_ADMIN_OPCODE_DELETE_CQ,
    NVME_CMD_ADMIN_OPCODE_CREATE_CQ = 5,
    NVME_CMD_ADMIN_OPCODE_IDENTIFY,

    NVME_CMD_ADMIN_OPCODE_ABORT = 8,
    NVME_CMD_ADMIN_OPCODE_SETFT,
    NVME_CMD_ADMIN_OPCODE_GETFT,
};

enum nvme_completion_queue_entry_status_status_code_type {
    NVME_COMPL_QUEUE_ENTRY_STATUS_STATUS_CODE_TYPE_GENERIC_CMD,
    NVME_COMPL_QUEUE_ENTRY_STATUS_STATUS_CODE_TYPE_CMD_SPECIFIC,
    NVME_COMPL_QUEUE_ENTRY_STATUS_STATUS_CODE_TYPE_MEDIA_DATA_INTEGRITY,
    NVME_COMPL_QUEUE_ENTRY_STATUS_STATUS_CODE_TYPE_PATH_RELATED,
};

enum nvme_completion_queue_entry_status_shifts {
    NVME_COMPL_QUEUE_ENTRY_STATUS_SHIFT_STATUS_CODE_SHIFT = 17,
    NVME_COMPL_QUEUE_ENTRY_STATUS_SHIFT_STATUS_CODE_TYPE_SHIFT = 25,
    NVME_COMPL_QUEUE_ENTRY_STATUS_SHIFT_RETRY_DELAY_SHIFT = 28
};

enum nvme_completion_queue_entry_status {
    __NVME_COMPL_QUEUE_ENTRY_STATUS_PHASE = 1 << 0,
    __NVME_COMPL_QUEUE_ENTRY_STATUS_CODE =
        0b11ull << NVME_COMPL_QUEUE_ENTRY_STATUS_SHIFT_STATUS_CODE_SHIFT,
    __NVME_COMPL_QUEUE_ENTRY_STATUS_CODE_TYPE =
        0xFFull << NVME_COMPL_QUEUE_ENTRY_STATUS_SHIFT_STATUS_CODE_TYPE_SHIFT,
    __NVME_COMPL_QUEUE_ENTRY_RETRY_DELAY =
        0b11ull << NVME_COMPL_QUEUE_ENTRY_STATUS_SHIFT_RETRY_DELAY_SHIFT,
    __NVME_COMPL_QUEUE_ENTRY_STATUS_MORE_INFO = 1ull << 30,
    __NVME_COMPL_QUEUE_ENTRY_STATUS_DONT_RETRY = 1ull << 31,
};

struct nvme_completion_queue_entry {
    uint32_t result;
    uint32_t unused;

    uint16_t sqhead;
    uint16_t sqid;
    uint16_t cid;
    uint16_t status;
};

enum nvme_command_feature {
    NVME_CMD_FEATURE_ARBITRATION = 1,
    NVME_CMD_FEATURE_POWER_MGMT,
    NVME_CMD_FEATURE_TEMP_THRESHOLD = 4,
    NVME_CMD_FEATURE_VOLATILE_WRITE_CACHE = 6,
    NVME_CMD_FEATURE_QUEUE_COUNT,
    NVME_CMD_FEATURE_INTERRUPT_COALESCING,
    NVME_CMD_FEATURE_INTERRUPT_VECTOR_COALESCING,
    NVME_CMD_FEATURE_ASYNC_EVENT_CONFIG = 11,
    NVME_CMD_FEATURE_ASYNC_POWER_STATE_TRANSITION,
    NVME_CMD_FEATURE_HOST_MEM_BUF,
    NVME_CMD_FEATURE_TIMESTAMP,
    NVME_CMD_FEATURE_KEEP_ALIVE_TIMER,
    NVME_CMD_FEATURE_HOST_CONTROLLED_THERM_MGMT,
    NVME_CMD_FEATURE_NON_OPER_POWER_STATE_CONFIG,
    NVME_CMD_FEATURE_READ_RECOVERY_LVL_CONFIG,
    NVME_CMD_FEATURE_PREDICTABLE_LATENCY_MODE_CONFIG,
    NVME_CMD_FEATURE_PREDICTABLE_LATENCY_MODE_WINDOW,
    NVME_CMD_FEATURE_HOST_BEHAVIOR_SUPPORT = 22,
    NVME_CMD_FEATURE_SANITIZE_CONFIG,
    NVME_CMD_FEATURE_ENDURANCE_GROUP_EVENT_CONFIG,
    NVME_CMD_FEATURE_IO_CMDSET_PROFILE,
    NVME_CMD_FEATURE_SPINUP_CONTROL,
    NVME_CMD_FEATURE_ENHANCED_CONTROLLER_METADATA = 75,
    NVME_CMD_FEATURE_CONTROLLER_METADATA,
    NVME_CMD_FEATURE_NAMESPACE_METADATA,
    NVME_CMD_FEATURE_SOFTWARE_PROGRESS_MARKER,
    NVME_CMD_FEATURE_HOST_IDENTIFIER,
    NVME_CMD_FEATURE_RESERVATION_NOTIF_MASK,
    NVME_CMD_FEATURE_RESERVATION_PERSISTENCE,
    NVME_CMD_FEATURE_NAMESPACE_WRITE_CONFIG,
};

enum nvme_create_cq_flags {
    __NVME_CREATE_CQ_PHYS_CONTIG = 1 << 0,
    __NVME_CREATE_CQ_IRQS_ENABLED = 1 << 1,
};

enum nvme_create_sq_flags {
    __NVME_CREATE_SQ_PHYS_CONTIG = 1 << 0,
};

struct nvme_command {
    union {
        struct {
            uint8_t opcode;
            uint8_t flags;
            uint16_t cid;
            uint32_t nsid;
            uint32_t cdw1[2];
            uint64_t metadata;
            uint64_t prp1;
            uint64_t prp2;
            uint32_t cdw2[6];
        } __packed common;
        struct {
            uint8_t opcode;
            uint8_t flags;
            uint16_t cid;
            uint32_t nsid;
            uint64_t unused;
            uint64_t metadata;
            uint64_t prp1;
            uint64_t prp2;
            uint64_t slba;
            uint16_t len;
            uint16_t control;
            uint32_t dsmgmt;
            uint32_t ref;
            uint16_t apptag;
            uint16_t appmask;
        } __packed readwrite;
        struct {
            uint8_t opcode;
            uint8_t flags;
            uint16_t cid;
            uint32_t nsid;
            uint64_t unused1;
            uint64_t unused2;
            uint64_t prp1;
            uint64_t prp2;
            uint32_t cns;
            uint32_t unused3[5];
        } __packed identify;
        struct {
            uint8_t opcode;
            uint8_t flags;
            uint16_t cid;
            uint32_t nsid;
            uint64_t unused1;
            uint64_t unused2;
            uint64_t prp1;
            uint64_t prp2;
            uint32_t fid;
            uint32_t dword;
            uint64_t unused[2];
        } __packed features;
        struct {
            uint8_t opcode;
            uint8_t flags;
            uint16_t cid;
            uint32_t unused1[5];
            uint64_t prp1;
            uint64_t unused2;
            uint16_t cqid;
            uint16_t size;
            uint16_t cqflags;
            uint16_t irqvec;
            uint64_t unused3[2];
        } __packed create_comp_queue;
        struct {
            uint8_t opcode;
            uint8_t flags;
            uint16_t cid;
            uint32_t unused1[5];
            uint64_t prp1;
            uint64_t unused2;
            uint16_t sqid;
            uint16_t size;
            uint16_t sqflags;
            uint16_t cqid;
            uint64_t unused3[2];
        } __packed create_submit_queue;
        struct {
            uint8_t opcode;
            uint8_t flags;
            uint16_t cid;
            uint32_t unused1[9];
            uint16_t qid;
            uint16_t unused2;
            uint32_t unused3[5];
        } __packed delete_queue;
        struct {
            uint8_t opcode;
            uint8_t flags;
            uint16_t cid;
            uint32_t unused1[9];
            uint16_t sqid;
            uint16_t cqid;
            uint32_t unused2[5];
        } abort;
    };
} __packed;
