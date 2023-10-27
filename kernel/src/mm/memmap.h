/*
 * kernel/mm/memmap.h
 * © suhas pai
 */

#pragma once
#include "lib/adt/range.h"

enum mm_memmap_kind {
    MM_MEMMAP_KIND_NONE,
    MM_MEMMAP_KIND_USABLE,
    MM_MEMMAP_KIND_RESERVED,
    MM_MEMMAP_KIND_ACPI_RECLAIMABLE,
    MM_MEMMAP_KIND_ACPI_NVS,
    MM_MEMMAP_KIND_BAD_MEMORY,
    MM_MEMMAP_KIND_BOOTLOADER_RECLAIMABLE,
    MM_MEMMAP_KIND_KERNEL_AND_MODULES,
    MM_MEMMAP_KIND_FRAMEBUFFER,
};

struct mm_memmap {
    struct range range;
    enum mm_memmap_kind kind;
};

const struct mm_memmap *mm_get_memmap_list();