/*
 * kernel/acpi/structs.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "lib/macros.h"

/* rsdp = "Root System Description Pointer" */

struct acpi_rsdp_v2_info {
    uint32_t length;
    uint64_t xsdt_addr;
    uint8_t ext_checksum;
    uint8_t reserved[3];
} __packed;

struct acpi_rsdp {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_addr;
    // ver 2.0 only
    struct acpi_rsdp_v2_info v2;
} __packed;

/* sdt = "System Description Table" */

struct acpi_sdt {
    char signature[4];
    uint32_t length;
    uint8_t rev;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_rev;
    uint32_t creator_id;
    uint32_t creator_revision;
} __packed;

/* rsdt = "Root System Description Table" */

struct acpi_rsdt {
    struct acpi_sdt sdt;
    char ptrs[];
} __packed;

struct acpi_madt {
    struct acpi_sdt sdt;
    uint32_t local_apic_base;
    uint32_t flags;
    char madt_entries[];
} __packed;

enum acpi_madt_entry_kind {
    ACPI_MADT_ENTRY_KIND_CPU_LOCAL_APIC,
    ACPI_MADT_ENTRY_KIND_IO_APIC,
    ACPI_MADT_ENTRY_KIND_INT_SRC_OVERRIDE,
    ACPI_MADT_ENTRY_KIND_NON_MASKABLE_INT_SRC,
    ACPI_MADT_ENTRY_KIND_NON_MASKABLE_INT,
    ACPI_MADT_ENTRY_KIND_LOCAL_APIC_ADDR_OVERRIDE,
    ACPI_MADT_ENTRY_KIND_CPU_LOCAL_X2APIC = 9,
    ACPI_MADT_ENTRY_KIND_CPU_LOCAL_X2APIC_NMI,
    ACPI_MADT_ENTRY_KIND_GIC_CPU_INTERFACE,
    ACPI_MADT_ENTRY_KIND_GIC_DISTRIBUTOR,
    ACPI_MADT_ENTRY_KIND_GIC_MSI_FRAME,
    ACPI_MADT_ENTRY_KIND_GIC_REDISTRIBUTOR,
    ACPI_MADT_ENTRY_KIND_GIC_INTR_TRANSLATE_SERVICE,
    ACPI_MADT_ENTRY_KIND_MULTIPROCESSOR_WAKEUP_SERVICE,

#if defined(__riscv64)
    ACPI_MADT_ENTRY_KIND_RISCV_HART_IRQ_CNTRLR = 24
#endif /* defined(__riscv64) */
};

struct acpi_madt_entry_header {
    enum acpi_madt_entry_kind kind : 8;
    uint8_t length;
} __packed;

enum acpi_madt_entry_cpu_lapic_flags {
    __ACPI_MADT_ENTRY_CPU_LAPIC_FLAG_ENABLED = 1 << 0,
    __ACPI_MADT_ENTRY_CPU_LAPIC_FLAG_ONLINE_CAPABLE = 1 << 1
};

struct acpi_madt_entry_cpu_lapic {
    struct acpi_madt_entry_header header;
    uint8_t processor_id;
    uint8_t apic_id;
    uint32_t flags;
} __packed;

struct acpi_madt_entry_ioapic {
    struct acpi_madt_entry_header header;
    uint8_t apic_id;
    uint8_t reserved;
    uint32_t base;
    uint32_t gsib; /* gsib = "Global System Interrupt Base" */
} __packed;

enum acpi_madt_entry_iso_flags {
    __ACPI_MADT_ENTRY_ISO_ACTIVE_LOW = 1 << 1,
    __ACPI_MADT_ENTRY_ISO_LEVEL_TRIGGER = 0b11 << 2,
};

struct acpi_madt_entry_iso {
    struct acpi_madt_entry_header header;
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t gsi; /* gsi = "Global System Interrupt" */
    uint16_t flags;
} __packed;

enum acpi_madt_entry_nmi_src_flags {
    __ACPI_MADT_ENTRY_NMI_SRC_ACTIVE_LOW = 1 << 1,
    __ACPI_MADT_ENTRY_NMI_SRC_LEVEL_TRIGGER = 1 << 4,
};

struct acpi_madt_entry_nmi_src {
    struct acpi_madt_entry_header header;

    uint8_t source;
    uint8_t reserved;
    uint16_t flags;
    uint32_t gsi; /* gsi = "Global System Interrupt" */
} __packed;

struct acpi_madt_entry_nmi {
    struct acpi_madt_entry_header header;
    uint8_t processor;
    uint16_t flags;
    uint8_t lint;
} __packed;

struct acpi_madt_entry_lapic_addr_override {
    uint16_t reserved;
    uint64_t base;
} __packed;

struct acpi_madt_entry_cpu_local_x2apic {
    uint16_t reserved;
    uint32_t x2apic_id;
    uint32_t flags;
    uint32_t acpi_uid;
} __packed;

struct acpi_madt_entry_cpu_local_x2apic_nmi {
    uint16_t reserved;
    uint32_t flags;
    uint32_t acpi_uid;
    uint8_t local_x2apic_lint;
    uint8_t reserved_2[3];
} __packed;

enum acpi_madt_entry_gic_cpu_flags {
    __ACPI_MADT_ENTRY_GIC_CPU_ENABLED = 1 << 0,
    __ACPI_MADT_ENTRY_GIC_CPU_PERF_INTR_EDGE_TRIGGER = 1 << 1,
    __ACPI_MADT_ENTRY_GIC_CPU_VGIC_INTR_EDGE_TRIGGER = 1 << 2,
};

enum acpi_madt_entry_gic_cpu_mpidr_flags {
    __ACPI_MADT_ENTRY_GIC_CPU_MPIDR_AFF0 = 0xff,
    __ACPI_MADT_ENTRY_GIC_CPU_MPIDR_AFF1 = 0xff << 8,
    __ACPI_MADT_ENTRY_GIC_CPU_MPIDR_AFF2 = 0xff << 16,
    __ACPI_MADT_ENTRY_GIC_CPU_MPIDR_AFF3 = 0xffull << 32,
};

struct acpi_madt_entry_gic_cpu_interface {
    struct acpi_madt_entry_header header;

    // Must be 0
    uint16_t reserved;

    uint32_t cpu_interface_number;
    uint32_t acpi_processor_id;
    uint32_t flags;

    uint32_t parking_protocol_version;
    uint32_t perf_interrupt_gsiv;
    uint64_t parked_address;
    uint64_t phys_base_address;
    uint64_t gic_virt_cpu_reg_address;
    uint64_t gic_virt_ctrl_block_reg_address;

    uint32_t vgic_maintenance_interrupt;
    uint64_t gicr_phys_base_address;
    uint64_t mpidr;

    // Lower class -> higher efficiency.
    uint8_t processor_power_efficiency_class;

    // Must be 0
    uint8_t reserved2;

    // SPE = Statistical Profiling Extension.
    // Statistical Profiling Extension buffer overflow GSIV. This interrupt is a
    // level triggered PPI. Zero if SPE is not supported by this processor.

    uint16_t spe_overflow_interrupt;
} __packed;

struct acpi_madt_entry_gic_distributor {
    struct acpi_madt_entry_header header;

    uint16_t reserved;
    uint32_t gic_hardware_id;
    uint64_t phys_base_address;
    uint32_t sys_vector_base;
    uint8_t gic_version;
    uint8_t reserved_2[3];
} __packed;

enum acpi_madt_entry_gic_msi_frame_flags {
    __ACPI_MADT_GICMSI_FRAME_OVERR_MSI_TYPERR = 1 << 0,
};

struct acpi_madt_entry_gic_msi_frame {
    struct acpi_madt_entry_header header;

    uint16_t reserved;
    uint32_t msi_frame_id;
    uint64_t phys_base_address;
    uint32_t flags;
    uint16_t spi_count;
    uint16_t spi_base;
} __packed;

enum acpi_gas_addrspace_kind {
    ACPI_GAS_ADDRSPACE_KIND_SYSMEM,
    ACPI_GAS_ADDRSPACE_KIND_SYS_IO,
    ACPI_GAS_ADDRSPACE_KIND_PCI_CONFIG,
    ACPI_GAS_ADDRSPACE_KIND_EMBED_CONTROLLER,
    ACPI_GAS_ADDRSPACE_KIND_SYS_MANAGEMENT_BUS,
    ACPI_GAS_ADDRSPACE_KIND_SYS_CMOS,
    ACPI_GAS_ADDRSPACE_KIND_SYS_PCI_DEV_BAR_TARGET,
    ACPI_GAS_ADDRSPACE_KIND_SYS_INTELLIGENT_PLATFORM_MANAGEMENT_INFRA,
    ACPI_GAS_ADDRSPACE_KIND_SYS_GEN_PURPOSE_IO,
    ACPI_GAS_ADDRSPACE_KIND_SYS_GEN_SERIAL_BUS,
    ACPI_GAS_ADDRSPACE_KIND_SYS_PLATFORM_COMM_CHANNEL,
};

enum acpi_gas_access_size_kind {
    ACPI_GAS_ACCESS_SIZE_UNDEFINED,

    ACPI_GAS_ACCESS_SIZE_1_BYTE,
    ACPI_GAS_ACCESS_SIZE_2_BYTE,
    ACPI_GAS_ACCESS_SIZE_4_BYTE,
    ACPI_GAS_ACCESS_SIZE_8_BYTE,
};

/* gas = Generic Address Structure */
struct acpi_gas {
    enum acpi_gas_addrspace_kind addr_space : 8;

    uint8_t bit_width;
    uint8_t bit_offset;

    enum acpi_gas_access_size_kind access_size : 8;
    uint64_t address;
} __packed;

enum acpi_fadt_preferred_pm_profile {
    ACPI_FADT_PREFERRED_PM_PROFILE_UNSPECIFIED,
    ACPI_FADT_PREFERRED_PM_PROFILE_DESKTOP,
    ACPI_FADT_PREFERRED_PM_PROFILE_MOBILE,
    ACPI_FADT_PREFERRED_PM_PROFILE_WORKSTATION,
    ACPI_FADT_PREFERRED_PM_PROFILE_ENTERPRISE_SERVER,
    ACPI_FADT_PREFERRED_PM_PROFILE_SOHO_SERVER,
    ACPI_FADT_PREFERRED_PM_PROFILE_APPLIANCE_PC,
    ACPI_FADT_PREFERRED_PM_PROFILE_PERFORMANCE_SERVER,
    ACPI_FADT_PREFERRED_PM_PROFILE_TABLET
};

enum acpi_fadt_flags {
    __ACPI_FADT_WBINVD                             = 1 << 0,
    __ACPI_FADT_WBINVD_FLUSH                       = 1 << 1,
    __ACPI_FADT_PROC_C1                            = 1 << 2,
    __ACPI_FADT_P_LVL2_UP                          = 1 << 3,
    __ACPI_FADT_PWR_BUTTON                         = 1 << 4,
    __ACPI_FADT_SLP_BUTTON                         = 1 << 5,
    __ACPI_FADT_FIX_RTC                            = 1 << 6,
    __ACPI_FADT_RTC_S4                             = 1 << 7,
    __ACPI_FADT_TMR_VAL_EXT                        = 1 << 8,
    __ACPI_FADT_DCK_CAP                            = 1 << 9,
    __ACPI_FADT_RESET_REG_SUP                      = 1 << 10,
    __ACPI_FADT_SEALED_CASE                        = 1 << 11,
    __ACPI_FADT_HEADLESS                           = 1 << 12,
    __ACPI_FADT_CPU_SW_SLP                         = 1 << 13,
    __ACPI_FADT_PCI_EXP_WAK                        = 1 << 14,
    __ACPI_FADT_USE_PLATFORM_CLOCK                 = 1 << 15,
    __ACPI_FADT_S4_RTC_STS_VALID                   = 1 << 16,
    __ACPI_FADT_REMOTE_POWER_ON                    = 1 << 17,
    __ACPI_FADT_FORCE_APIC_CLUSTER                 = 1 << 18,
    __ACPI_FADT_FORCE_APIC_PHYS_DEST_MODE          = 1 << 19,
    __ACPI_FADT_FORCE_HW_REDUCED_ACPI              = 1 << 20,
    __ACPI_FADT_FORCE_HW_LOW_POWER_S0_IDLE_CAPABLE = 1 << 21,
};

enum acpi_fadt_iapc_boot_flags {
    __ACPI_FADT_IAPC_BOOT_LEGACY_DEVICES          = 1 << 0,
    __ACPI_FADT_IAPC_BOOT_8042                    = 1 << 1,
    __ACPI_FADT_IAPC_BOOT_VGA_NOT_PRESENT         = 1 << 2,
    __ACPI_FADT_IAPC_BOOT_MSI_NOT_SUPPORTED       = 1 << 3,
    __ACPI_FADT_IAPC_BOOT_PCIe_ASPM_NOT_SUPPORTED = 1 << 4,
    __ACPI_FADT_IAPC_BOOT_CMOS_NOT_PRESENT        = 1 << 5,
};

enum acpi_fadt_arm_boot_flags {
    __ACPI_FADT_ARM_BOOT_PSCI_COMPLIANT = 1 << 0,
    __ACPI_FADT_ARM_BOOT_PSCI_USE_HVC = 1 << 1,
};

enum acpi_fadt_pm1_status {
    /*
     * This bit gets set any time the most significant bit of a 24/32-bit
     * counter changes from clear to set or set to clear.
     */

    __ACPI_FADT_PM1_STATUS_TIMER_CARRY_STATUS = 1 << 0,

    /*
     * This is the bus master status bit. This bit is set any time a system bus
     * master requests the system bus, and can only be cleared by writing a "1"
     * to this bit position. Notice that this bit reflects bus master activity,
     * not CPU activity (this bit monitors any bus master that can cause an
     * incoherent cache for a processor in the C3 state when the bus master
     * performs a memory transaction).
     */

    __ACPI_FADT_PM1_STATUS_BUS_MASTER_STATUS = 1 << 4,

    /*
     * This bit is set when an SCI is generated due to the platform runtime
     * firmware wanting the attention of the SCI handler. Platform runtime
     * firmware will have a control bit (somewhere within its address space)
     * that will raise an SCI and set this bit. This bit is set in response to
     * the platform runtime firmware releasing control of the Global Lock and
     * having seen the pending bit set.
     */

    __ACPI_FADT_PM1_STATUS_GBL_STATUS = 1 << 5,

    /*
     * This optional bit is set when the Power Button is pressed. In the system
     * working state, while PWRBTN_EN and PWRBTN_STS are both set, an interrupt
     * event is raised. In the sleep or soft-off state, a wake event is
     * generated when the power button is pressed (regardless of the PWRBTN_EN
     * bit setting). This bit is only set by hardware and can only be reset by
     * software writing a "1" to this bit position.
     *
     * ACPI defines an optional mechanism for unconditional transitioning a
     * system that has stopped working from the G0 working state into the G2
     * soft-off state called the power button override. If the Power Button is
     * held active for more than four seconds, this bit is cleared by hardware
     * and the system transitions into the G2/S5 Soft Off state
     * (unconditionally).
     *
     * Support for the power button is indicated by the PWR_BUTTON flag in the
     * FADT being reset (zero). If the PWR_BUTTON flag is set or a power button
     * device object is present in the ACPI Namespace, then this bit field is
     * ignored by OSPM. If the power button was the cause of the wake (from an
     * S1-S4 state), then this bit is set prior to returning control to OSPM.
     */

    __ACPI_FADT_PM1_STATUS_PWR_BTN_STATUS = 1 << 8,

    /*
    * This optional bit is set when the sleep button is pressed. In the system
    * working state, while SLPBTN_EN and SLPBTN_STS are both set, an interrupt
    * event is raised. In the sleep or soft-off states a wake event is generated
    * when the sleeping button is pressed and the SLPBTN_EN bit is set. This bit
    * is only set by hardware and can only be reset by software writing a "1" to
    * this bit position.
    *
    * Support for the sleep button is indicated by the SLP_BUTTON flag in the
    * FADT being reset (zero). If the SLP_BUTTON flag is set or a sleep button
    * device object is present in the ACPI Namespace, then this bit field is
    * ignored by OSPM.
    *
    * If the sleep button was the cause of the wake (from an S1-S4 state), then
    * this bit is set prior to returning control to OSPM.
    */

    __ACPI_FADT_PM1_STATUS_SLP_BTN_STATUS = 1 << 9,

    /*
     * This optional bit is set when the RTC generates an alarm (asserts the RTC
     * IRQ signal). Additionally, if the RTC_EN bit is set then the setting of
     * the RTC_STS bit will generate a power management event (an SCI, SMI, or
     * resume event). This bit is only set by hardware and can only be reset by
     * software writing a "1" to this bit position.
     *
     * If the RTC was the cause of the wake (from an S1-S3 state), then this bit
     * is set prior to returning control to OSPM. If the RTC_S4 flag within the
     * FADT is set, and the RTC was the cause of the wake from the S4 state),
     * then this bit is set prior to returning control to OSPM.
     */

    __ACPI_FADT_PM1_STATUS_RTC_STS = 1 << 10,

    /*
     * This bit is optional for chipsets that implement PCI Express.
     *
     * This bit is set by hardware to indicate that the system woke due to a PCI
     * Express wakeup event. A PCI Express wakeup event is defined as the PCI
     * Express WAKE# pin being active , one or more of the PCI Express ports
     * being in the beacon state, or receipt of a PCI Express PME message at a
     * root port. This bit should only be set when one of these events causes
     * the system to transition from a non-S0 system power state to the S0
     * system power state. This bit is set independent of the state of the
     * PCIEXP_WAKE_DIS bit.
     *
     * Software writes a 1 to clear this bit. If the WAKE# pin is still active
     * during the write, one or more PCI Express ports is in the beacon state or
     * the PME message received indication has not been cleared in the root
     * port, then the bit will remain active (i.e. all inputs to this bit are
     * level-sensitive).
     *
     * Note: This bit does not itself cause a wake event or prevent entry to a
     * sleeping state. Thus if the bit is 1 and the system is put into a
     * sleeping state, the system will not automatically wake.
     */

    __ACPI_FADT_PM1_STATUS_PCIEXP_WAKE_STS = 1 << 14,

    /*
     * This bit is set when the system is in the sleeping state and an enabled
     * wake event occurs. Upon setting this bit system will transition to the
     * working state. This bit is set by hardware and can only be cleared by
     * software writing a "1" to this bit position.
     */

    __ACPI_FADT_PM1_STATUS_WAKE_STATUS = 1 << 15,
};

/*
 * Attribute: Read/Write
 *
 * The PM1 control registers contain the fixed hardware feature control bits.
 * These bits can be split between two registers: PM1a_CNT or PM1b_CNT. Each
 * register grouping can be at a different 32-bit aligned address and is pointed
 * to by the PM1a_CNT_BLK or PM1b_CNT_BLK. The values for these pointers to the
 * register space are found in the FADT. Accesses to PM1 control registers are
 * accessed through byte and word accesses.
 *
 * This register contains optional features enabled or disabled within the FADT.
 * If the FADT indicates that the feature is not supported as a fixed hardware
 * feature, then software treats these bits as ignored.
 */

enum acpi_fadt_pm1_enable_registers {
    __ACPI_FADT_PM1_ENABLE_TMR_EN     = 1 << 0,
    __ACPI_FADT_PM1_ENABLE_GBL_EN     = 1 << 5,
    __ACPI_FADT_PM1_ENABLE_PWR_BTN_EN = 1 << 8,
    __ACPI_FADT_PM1_ENABLE_SLP_BTN_EN = 1 << 9,
    __ACPI_FADT_PM1_ENABLE_RTC_EN     = 1 << 10,

    /*
     * This bit disables the inputs to the PCIEXP_WAKE_STS bit in the PM1 Status
     * register from waking the system. Modification of this bit has no impact
     * on the value of the PCIEXP_WAKE_STS bit. PCIEXP_WAKE_DIS bit. Software
     * writes a 1 to clear this bit. If the WAKE# pin is still active during the
     * write, one or more PCI Express ports is in the beacon state or the PME
     * message received indication has not been cleared in the root port, then
     * the bit will remain active (i.e. all inputs to this bit are
     * level-sensitive). Note: This bit does not itself cause a wake event or
     * prevent entry to a sleeping state. Thus if the bit is 1 and the system is
     * put into a sleeping state, the system will not automatically wake.
     */

    __ACPI_FADT_PM1_ENABLE_PCIEXP_WAKE_EN = 1 << 14,
};

enum acpi_fadt_pm1_control_registers {
    /*
     * Selects the power management event to be either an SCI or SMI interrupt
     * for the following events. When this bit is set, then power management
     * events will generate an SCI interrupt. When this bit is reset power
     * management events will generate an SMI interrupt. It is the
     * responsibility of the hardware to set or reset this bit. OSPM always
     * preserves this bit position.
     */

    __ACPI_FADT_PM1_CONTROL_SCI_EN = 1 << 0,

    /*
     * When set, this bit allows the generation of a bus master request to cause
     * any processor in the C3 state to transition to the C0 state. When this
     * bit is reset, the generation of a bus master request does not affect any
     * processor in the C3 state.
     */

    __ACPI_FADT_PM1_CONTROL_BM_RLD = 1 << 1,

    /*
     * This write-only bit is used by the ACPI software to raise an event to the
     * platform runtime firmware, that is, generates an SMI to pass execution
     * control to the platform runtime firmware for IA-PC platforms. Platform
     * runtime firmware software has a corresponding enable and status bit to
     * control its ability to receive ACPI events (for example, BIOS_EN and
     * BIOS_STS). The GBL_RLS bit is set by OSPM to indicate a release of the
     * Global Lock and the setting of the pending bit in the FACS memory
     * structure.
     */

    __ACPI_FADT_PM1_CONTROL_GBL_RLS = 1 << 2,

    /*
     * Defines the type of sleeping or soft-off state the system enters when the
     * SLP_EN bit is set to one. This 3-bit field defines the type of hardware
     * sleep state the system enters when the SLP_EN bit is set. The \_Sx object
     * contains 3-bit binary values associated with the respective sleeping
     * state (as described by the object). OSPM takes the two values from the
     * \_Sx object and programs each value into the respective SLP_TYPx field.
     */

    __ACPI_FADT_PM1_CONTROL_SLP_TYP = 0b111ull << 10,

    /*
     * This is a write-only bit and reads to it always return a zero. Setting
     * this bit causes the system to sequence into the sleeping state associated
     * with the SLP_TYPx fields programmed with the values from the \_Sx object.
     */

    __ACPI_FADT_PM1_CONTROL_SLP_EN = 1 << 13,
};

struct acpi_fadt {
    struct acpi_sdt sdt;

    uint32_t firmware_ctrl;
    uint32_t dsdt;

    /* field used in ACPI 1.0; no longer in use, for compatibility only */
    uint8_t reserved;
    uint8_t preferred_power_management_profile;

    uint16_t sci_interrupt;
    uint32_t smi_commandport;

    uint8_t acpi_enable;
    uint8_t acpi_disable;

    uint8_t s4bios_req;

    uint8_t pstate_control;
    uint32_t pm1a_event_block;
    uint32_t pm1b_event_block;
    uint32_t pm1a_control_block;
    uint32_t pm1b_control_block;
    uint32_t pm2_control_block;
    uint32_t pm_timer_block;

    uint32_t gpe0_block;
    uint32_t gpe1_block;

    uint8_t pm1_event_length;
    uint8_t pm1_control_length;
    uint8_t pm2_control_length;
    uint8_t pm_timer_length;

    uint8_t gpe0_length;
    uint8_t gpe1_length;
    uint8_t gpe1_base;

    uint8_t cstate_control;

    uint16_t worst_c2_latency;
    uint16_t worst_c3_latency;

    uint16_t flush_size;
    uint16_t flush_stride;

    uint8_t duty_offset;
    uint8_t duty_width;

    /*
     * Eight bit value that can represent 0x01-0x31 days in BCD or 0x01-0x1F
     * days in binary. Bits 6 and 7 of this field are treated as Ignored by
     * software. The RTC is initialized such that this field contains a "don't
     * care" value when the platform firmware switches from legacy to ACPI mode.
     * A don’t care value can be any unused value (not 0x1-0x31 BCD or 0x01-0x1F
     * hex) that the RTC reverts back to a 24 hour alarm.
     */

    uint8_t day_alarm;

    /*
     * Eight bit value that can represent 01-12 months in BCD or 0x01-0xC months
     * in binary. The RTC is initialized such that this field contains a don't
     * care value when the platform firmware switches from legacy to ACPI mode.
     * A "don't care" value can be any unused value (not 1-12 BCD or x01-xC hex)
     * that the RTC reverts back to a 24 hour alarm and/or 31 day alarm).
     */

    uint8_t month_alarm;

    /*
     * 8-bit BCD or binary value. This value indicates the thousand year and
     * hundred year (Centenary) variables of the date in BCD (19 for this
     * century, 20 for the next) or binary (x13 for this century, x14 for the
     * next).
     */

    uint8_t century;
    uint16_t iapc_boot_arch_flags;

    uint8_t reserved2;
    uint32_t flags;

    struct acpi_gas reset_reg;

    uint8_t reset_value;
    uint16_t arm_boot_arch_flags;
    uint8_t fadt_minor_version;

    // Available on ACPI 2.0+
    uint64_t x_firmware_ctrl;
    uint64_t x_dsdt;

    struct acpi_gas x_pm1a_event_block;
    struct acpi_gas x_pm1b_event_block;
    struct acpi_gas x_pm1a_ctrl_block;
    struct acpi_gas x_pm1b_ctrl_block;
    struct acpi_gas x_pm2_ctrl_block;
    struct acpi_gas x_pm_timer_block;
    struct acpi_gas x_gpe0_block;
    struct acpi_gas x_gpe1_block;
    struct acpi_gas sleep_control_reg;
    struct acpi_gas sleep_status_reg;
    uint64_t hypervisor_vendor_identity;
} __packed;

enum acpi_fadt_pm2_control_registers {
    __ACPI_FADT_PM2_CONTROL_ABR_DISABLE = 1 << 0,
};

struct acpi_mcfg_entry {
    uint64_t base_addr;
    uint16_t segment_num;

    uint8_t bus_start_num;
    uint8_t bus_end_num;
} __packed;

struct acpi_mcfg {
    struct acpi_sdt sdt;
    uint64_t reserved;

    struct acpi_mcfg_entry entries[];
} __packed;