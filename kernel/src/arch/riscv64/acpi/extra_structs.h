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
