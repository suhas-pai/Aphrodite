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
    ACPI_RHCT_NODE_KIND_HART_INFO = 65535,
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
} __packed;