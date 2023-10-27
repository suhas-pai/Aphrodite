/*
 * kernel/arch/aarch64/mm/types.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lib/macros.h"

#define MAX_ORDER 31

#define PML1_SHIFT 12
#define PML2_SHIFT 21
#define PML3_SHIFT 30
#define PML4_SHIFT 39
#define PML5_SHIFT 48

#define PAGE_SHIFT PML1_SHIFT
#define PTE_PHYS_MASK 0x0000fffffffff000

#define PGT_LEVEL_COUNT 5
#define PGT_PTE_COUNT 512

#define PML1_MASK 0x1ff
#define PML2_MASK PML1_MASK
#define PML3_MASK PML1_MASK
#define PML4_MASK PML1_MASK
#define PML5_MASK 0xf

#define PML1(phys) (((phys) >> PML1_SHIFT) & PML1_MASK)
#define PML2(phys) (((phys) >> PML2_SHIFT) & PML2_MASK)
#define PML3(phys) (((phys) >> PML3_SHIFT) & PML3_MASK)
#define PML4(phys) (((phys) >> PML4_SHIFT) & PML4_MASK)
#define PML5(phys) (((phys) >> PML5_SHIFT) & PML5_MASK)

#define pte_to_phys(pte) ((pte) & PTE_PHYS_MASK)
#define phys_create_pte(phys) (phys)

typedef uint64_t pte_t;

static const uint16_t PT_LEVEL_MASKS[PGT_LEVEL_COUNT + 1] =
    { (1ull << 12) - 1, PML1_MASK, PML2_MASK, PML3_MASK, PML4_MASK, PML5_MASK };

static const uint8_t PAGE_SHIFTS[PGT_LEVEL_COUNT] =
    { PML1_SHIFT, PML2_SHIFT, PML3_SHIFT, PML4_SHIFT, PML5_SHIFT };

static const uint8_t LARGEPAGE_LEVELS[] = { 2, 3, 4 };
static const uint8_t LARGEPAGE_SHIFTS[] =
    { PML2_SHIFT, PML3_SHIFT, PML4_SHIFT };

#define PAGE_SIZE_2MIB (1ull << PML2_SHIFT)
#define PAGE_SIZE_1GIB (1ull << PML3_SHIFT)
#define PAGE_SIZE_512GIB (1ull << PML4_SHIFT)

#define LARGEPAGE_LEVEL_2MIB 2
#define LARGEPAGE_LEVEL_1GIB 3
#define LARGEPAGE_LEVEL_512GIB 4

struct largepage_level_info {
    uint8_t order;
    uint8_t level; // can't use pgt_level_t
    uint8_t largepage_order;
    bool is_supported : 1;

    uint64_t size;
};

extern struct largepage_level_info largepage_level_info_list[PGT_LEVEL_COUNT];

#define PAGE_SIZE_AT_LEVEL(level) \
    ({\
        const uint64_t __sizes__[] = { \
            PAGE_SIZE,      \
            PAGE_SIZE_2MIB, \
            PAGE_SIZE_1GIB, \
            /* The following aren't valid largepage sizes, but keep them */ \
            PAGE_SIZE_1GIB * PGT_PTE_COUNT, \
            PAGE_SIZE_1GIB * PGT_PTE_COUNT * PGT_PTE_COUNT \
        }; \
       __sizes__[level - 1];\
    })

enum pte_flags {
    __PTE_VALID  = 1ull << 0,
    __PTE_4KPAGE = 1ull << 1, // Valid only on ptes of a pml1 table
    __PTE_TABLE  = 1ull << 1,

    __PTE_WC = 1ull << 2,
    __PTE_WT = 2ull << 2,

    __PTE_MMIO = 3ull << 2, // Device uncacheable memory
    __PTE_USER = 1ull << 6,
    __PTE_RO = 1ull << 7,

    __PTE_UNPREDICT = 0b01ull << 8,
    __PTE_OUTER_SH = 0b10ull << 8,
    __PTE_INNER_SH = 0b11ull << 8,

    __PTE_ACCESS = 1ull << 10,
    __PTE_NONGLOBAL = 1ull << 11,

    __PTE_DIRTY = 1ull << 51,
    __PTE_CONTIG = 1ull << 52,
    __PTE_PXN = 1ull << 53,
    __PTE_UXN = 1ull << 54,
};

#define PGT_FLAGS (__PTE_VALID | __PTE_TABLE)
#define PTE_LARGE_FLAGS(level) \
    ({ (void)(level); __PTE_VALID | __PTE_ACCESS; })

#define PTE_LEAF_FLAGS (__PTE_VALID | __PTE_4KPAGE | __PTE_ACCESS)

struct page;