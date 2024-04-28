/*
 * kernel/src/arch/riscv64/acpi/structs.h
 * © suhas pai
 */

#pragma once
#include "acpi/structs.h"

struct acpi_rhct {
    struct acpi_sdt sdt;
    uint32_t reserved;
    uint64_t time_base_freq;
    uint32_t node_count;
    uint32_t node_offset;
} __packed;

enum acpi_rhct_node_kind {
    ACPI_RHCT_NODE_KIND_ISA_STRING,
    ACPI_RHCT_NODE_KIND_CMO,
    ACPI_RHCT_NODE_KIND_MMU,
    ACPI_RHCT_NODE_KIND_HART_INFO = 0xFFFF,
};

struct acpi_rhct_node {
    uint16_t kind;
    uint16_t length;
    uint16_t revision;
};

struct acpi_rhct_isa_string {
    struct acpi_rhct_node node;
    uint16_t isa_length;
    char isa_string[];
} __packed;

struct acpi_rhct_cmo_node {
    struct acpi_rhct_node node;

    uint8_t reserved;
    uint8_t cbom_shift;
    uint8_t cbop_shift;
    uint8_t cboz_shift;
} __packed;

enum acpi_rhct_mmu_kind {
    ACPI_RHCT_MMU_KIND_SV39,
    ACPI_RHCT_MMU_KIND_SV48,
    ACPI_RHCT_MMU_KIND_SV57
};

struct acpi_rhct_mmu_node {
    struct acpi_rhct_node node;

    uint8_t reserved;
    uint8_t mmu_kind;
} __packed;

struct acpi_rhct_hart_info {
    struct acpi_rhct_node node;

    uint16_t offset_count;
    uint32_t acpi_processor_uid;
    uint32_t offsets[];
} __packed;

enum acpi_madt_riscv_hart_irq_controller_flags {
    __ACPI_MADT_RISCV_HART_IRQ_CNTRLR_ENABLED = 1 << 0,
    __ACPI_MADT_RISCV_HART_IRQ_ONLINE_CAPABLE = 1 << 1,
};

struct acpi_madt_riscv_hart_irq_controller {
    struct acpi_madt_entry_header header;

    uint8_t version;
    uint8_t reserved;
    uint32_t flags;
    uint64_t hart_id;
    uint32_t acpi_proc_uid;

    uint32_t external_irq_controller_id;
    uint64_t imsic_base_addr;
    uint32_t imsic_size;
} __packed;

struct acpi_madt_riscv_imsic {
    struct acpi_madt_entry_header header;

    uint8_t version;
    uint32_t flags;
    uint16_t guest_node_irq_identity_count;

    uint8_t guest_index_bits;
    uint8_t hart_index_bits;
    uint8_t group_index_bits;
    uint8_t group_index_shift;
} __packed;

struct acpi_madt_riscv_aplic {
    struct acpi_madt_entry_header header;

    uint8_t version;
    uint8_t id;
    uint32_t flags;
    uint64_t hardware_id;
    uint16_t idc_count;
    uint16_t ext_irq_source_count;

    uint32_t gsi_base;

    uint64_t aplic_base;
    uint32_t aplic_size;
} __packed;

struct acpi_madt_riscv_plic {
    struct acpi_madt_entry_header header;

    uint8_t version;
    uint8_t id;
    uint64_t hardware_id;
    uint16_t total_ext_irq_source_supported;
    uint16_t max_prio;
    uint32_t flags;

    uint32_t plic_size;
    uint64_t plic_base;

    uint32_t gsi_base;
} __packed;
