/*
 * kernel/src/arch/aarch64/mm/types.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lib/assert.h"

#define MAX_ORDER 31

#if defined(AARCH64_USE_16K_PAGES)
    #define PML1_SHIFT 14ul
    #define PML2_SHIFT 25ul
    #define PML3_SHIFT 36ul
    #define PML4_SHIFT 47ul

    #define PGT_LEVEL_COUNT 4ul
    #define PGT_PTE_COUNT(level) ((level) != 4 ? (uint16_t)2048 : (uint16_t)2)

    #define PML1_MASK 0xbull
    #define PML2_MASK PML1_MASK
    #define PML3_MASK PML1_MASK
    #define PML4_MASK 0x1
#else
    #define PML1_SHIFT 12ul
    #define PML2_SHIFT 21ul
    #define PML3_SHIFT 30ul
    #define PML4_SHIFT 39ul
    #define PML5_SHIFT 48ul

    #define PGT_LEVEL_COUNT 5ul
    #define PGT_PTE_COUNT(level) ({ (void)(level); (uint16_t)512; })

    #define PML1_MASK 0x1ffull
    #define PML2_MASK PML1_MASK
    #define PML3_MASK PML1_MASK
    #define PML4_MASK PML1_MASK
    #define PML5_MASK 0xf
#endif /* defined(AARCH64_USE_16K_PAGES) */

#define PTE_PHYS_MASK 0x0000fffffffff000ull
#define PAGE_SHIFT PML1_SHIFT

#define PML1(phys) (((phys) >> PML1_SHIFT) & PML1_MASK)
#define PML2(phys) (((phys) >> PML2_SHIFT) & PML2_MASK)
#define PML3(phys) (((phys) >> PML3_SHIFT) & PML3_MASK)
#define PML4(phys) (((phys) >> PML4_SHIFT) & PML4_MASK)

#if !defined(AARCH64_USE_16K_PAGES)
    #define PML5(phys) (((phys) >> PML5_SHIFT) & PML5_MASK)
#endif /* defined(AARCH64_USE_16K_PAGES) */

#define pte_to_phys(pte) ((pte) & PTE_PHYS_MASK)
#define phys_create_pte(phys) ((pte_t)phys)

typedef uint64_t pte_t;

static const uint16_t PT_LEVEL_MASKS[PGT_LEVEL_COUNT + 1] = {
    (1ull << PML1_SHIFT) - 1, PML1_MASK, PML2_MASK, PML3_MASK, PML4_MASK,
#if !defined(AARCH64_USE_16K_PAGES)
    PML5_MASK
#endif /* defined(AARCH64_USE_16K_PAGES) */
};

static const uint8_t PAGE_SHIFTS[PGT_LEVEL_COUNT] = {
    PML1_SHIFT, PML2_SHIFT, PML3_SHIFT, PML4_SHIFT,
#if !defined(AARCH64_USE_16K_PAGES)
    PML5_SHIFT
#endif /* defined(AARCH64_USE_16K_PAGES) */
};

static const uint8_t LARGEPAGE_LEVELS[] = { 2, 3, 4 };
static const uint8_t LARGEPAGE_SHIFTS[] = {
    PML2_SHIFT, PML3_SHIFT, PML4_SHIFT
};

#if defined(AARCH64_USE_16K_PAGES)
    #define PAGE_SIZE_32MIB (1ull << PML2_SHIFT)
    #define PAGE_SIZE_64GIB (1ull << PML3_SHIFT)
    #define PAGE_SIZE_128TIB (1ull << PML4_SHIFT)

    #define LARGEPAGE_LEVEL_32MIB 2
    #define LARGEPAGE_LEVEL_64GIB 3
    #define LARGEPAGE_LEVEL_128TIB 4
#else
    #define PAGE_SIZE_2MIB (1ull << PML2_SHIFT)
    #define PAGE_SIZE_1GIB (1ull << PML3_SHIFT)
    #define PAGE_SIZE_512GIB (1ull << PML4_SHIFT)

    #define LARGEPAGE_LEVEL_2MIB 2
    #define LARGEPAGE_LEVEL_1GIB 3
    #define LARGEPAGE_LEVEL_512GIB 4
#endif /* defined(AARCH64_USE_16K_PAGES) */

struct largepage_level_info {
    uint8_t order;
    uint8_t level; // can't use pgt_level_t
    uint8_t largepage_order;
    bool is_supported : 1;

    uint64_t size;
};

extern struct largepage_level_info largepage_level_info_list[PGT_LEVEL_COUNT];

#if defined(AARCH64_USE_16K_PAGES)
    #define PAGE_SIZE_AT_LEVEL(level) \
        ({\
            const uint64_t __sizes__[] = { \
                PAGE_SIZE, \
                PAGE_SIZE_32MIB, \
                PAGE_SIZE_64GIB, \
                PAGE_SIZE_128TIB, \
                PAGE_SIZE_128TIB * PGT_PTE_COUNT(4) \
            }; \
        __sizes__[level - 1];\
        })
    #define PAGE_SIZE_AT_LEVEL(level) ({ \
        __auto_type __pagesizelevelresult__ = (uint64_t)0; \
        switch (level) { \
            case 1: \
                __pagesizelevelresult__ = PAGE_SIZE; \
                break; \
            case 2: \
                __pagesizelevelresult__ = PAGE_SIZE_32MIB; \
                break; \
            case 3: \
                __pagesizelevelresult__ = PAGE_SIZE_64GIB; \
                break; \
            case 4: \
                __pagesizelevelresult__ = PAGE_SIZE_128TIB; \
                break; \
            case 5: \
                __pagesizelevelresult__ = 1ull << PML5_SHIFT; \
                break; \
            default: \
                verify_not_reached(); \
        } \
        __pagesizelevelresult__; \
    })
#else
    #define PAGE_SIZE_AT_LEVEL(level) ({ \
        __auto_type __pagesizelevelresult__ = (uint64_t)0; \
        switch (level) { \
            case 1: \
                __pagesizelevelresult__ = PAGE_SIZE; \
                break; \
            case 2: \
                __pagesizelevelresult__ = PAGE_SIZE_2MIB; \
                break; \
            case 3: \
                __pagesizelevelresult__ = PAGE_SIZE_1GIB; \
                break; \
            case 4: \
                __pagesizelevelresult__ = 1ull << PML4_SHIFT; \
                break; \
            case 5: \
                __pagesizelevelresult__ = 1ull << PML5_SHIFT; \
                break; \
            default: \
                verify_not_reached(); \
        } \
        __pagesizelevelresult__; \
    })
#endif /* defined(AARCH64_USE_16K_PAGES) */

enum pte_flags {
    __PTE_VALID  = 1ull << 0,
    __PTE_PML1_PAGE = 1ull << 1, // Valid only on ptes of a pml1 table
    __PTE_TABLE = 1ull << 1,

    __PTE_WC = 1ull << 2,
    __PTE_WT = 0b10ull << 2,

    __PTE_MMIO = 0b11ull << 2, // Device uncacheable memory
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
#define PTE_LARGE_FLAGS(level) ({ (void)(level); __PTE_VALID | __PTE_ACCESS; })

#define PTE_LEAF_FLAGS (__PTE_VALID | __PTE_PML1_PAGE | __PTE_ACCESS)

struct page;