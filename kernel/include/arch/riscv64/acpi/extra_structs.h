/*
 * kernel/include/arch/riscv64/acpi/structs.h
 * © suhas pai
 */

#pragma once
#include "acpi/structs.h"

struct os_acpi_rhct {
    struct os_acpi_sdt sdt;
    uint32_t reserved;
    uint64_t time_base_freq;
    uint32_t node_count;
    uint32_t node_offset;
} __packed;

enum acpi_rhct_node_kind : uint16_t {
    OS_ACPI_RHCT_NODE_KIND_ISA_STRING,
    OS_ACPI_RHCT_NODE_KIND_CMO,
    OS_ACPI_RHCT_NODE_KIND_MMU,
    OS_ACPI_RHCT_NODE_KIND_HART_INFO = 0xFFFF,
};

struct os_acpi_rhct_node {
    uint16_t kind;
    uint16_t length;
    uint16_t revision;
};

struct os_acpi_rhct_isa_string {
    struct os_acpi_rhct_node node;
    uint16_t isa_length;
    char isa_string[];
} __packed;

struct os_acpi_rhct_cmo_node {
    struct os_acpi_rhct_node node;

    uint8_t reserved;
    uint8_t cbom_shift;
    uint8_t cbop_shift;
    uint8_t cboz_shift;
} __packed;

enum acpi_rhct_mmu_kind : uint8_t {
    OS_ACPI_RHCT_MMU_KIND_SV39,
    OS_ACPI_RHCT_MMU_KIND_SV48,
    OS_ACPI_RHCT_MMU_KIND_SV57
};

struct os_acpi_rhct_mmu_node {
    struct os_acpi_rhct_node node;

    uint8_t reserved;
    uint8_t mmu_kind;
} __packed;

struct os_acpi_rhct_hart_info {
    struct os_acpi_rhct_node node;

    uint16_t offset_count;
    uint32_t acpi_processor_uid;
    uint32_t offsets[];
} __packed;
