/*
 * kernel/include/mm/memmap.h
 * © suhas pai
 */

#pragma once
#include "lib/adt/range.h"

enum mm_memmap_kind : uint8_t {
    MM_MEMMAP_KIND_NONE,
    MM_MEMMAP_KIND_USABLE,
    MM_MEMMAP_KIND_RESERVED,
    MM_MEMMAP_KIND_ACPI_RECLAIMABLE,
    MM_MEMMAP_KIND_ACPI_NVS,
    MM_MEMMAP_KIND_BAD_MEMORY,
    MM_MEMMAP_KIND_BOOTLOADER_RECLAIMABLE,
    MM_MEMMAP_KIND_EXEC_AND_MODULES,
    MM_MEMMAP_KIND_FRAMEBUFFER,
};

struct mm_memmap {
    struct range range;
    enum mm_memmap_kind kind;
};

#define mm_for_each_memmap(memmap) \
    ptrarr_foreach(mm_get_memmap_list(), mm_get_memmap_count(), memmap)

const struct mm_memmap *mm_get_memmap_list();
uint8_t mm_get_memmap_count();
