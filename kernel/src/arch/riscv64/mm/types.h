/*
 * kernel/src/arch/riscv64/mm/types.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lib/assert.h"

#define MAX_ORDER 31ul

#define PML1_SHIFT 12ul
#define PML2_SHIFT 21ul
#define PML3_SHIFT 30ul
#define PML4_SHIFT 39ul
#define PML5_SHIFT 48ul

#define PAGE_SHIFT PML1_SHIFT
#define PTE_PHYS_MASK 0x003ffffffffffc00ull

#define PGT_LEVEL_COUNT 5ul
#define PGT_PTE_COUNT(level) ({ (void)(level); (uint16_t)512; })

#define PML1_MASK 0x1ffull
#define PML2_MASK PML1_MASK
#define PML3_MASK PML1_MASK
#define PML4_MASK PML1_MASK
#define PML5_MASK PML1_MASK

#define PML1(phys) (((phys) >> PML1_SHIFT) & PML1_MASK)
#define PML2(phys) (((phys) >> PML2_SHIFT) & PML2_MASK)
#define PML3(phys) (((phys) >> PML3_SHIFT) & PML3_MASK)
#define PML4(phys) (((phys) >> PML4_SHIFT) & PML4_MASK)
#define PML5(phys) (((phys) >> PML5_SHIFT) & PML5_MASK)

#define pte_to_phys(pte, level) ({ \
    (void)(level); \
    ((pte) & PTE_PHYS_MASK) << 2; \
})

#define phys_create_pte(phys) ((pte_t)(phys) >> 2)

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
    uint8_t largepage_order;
    uint8_t level; // can't use pgt_level_t

    bool is_supported : 1;
    uint64_t size;
};

extern struct largepage_level_info largepage_level_info_list[PGT_LEVEL_COUNT];

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
            __pagesizelevelresult__ = PAGE_SIZE_512GIB; \
            break; \
        case 5: \
            __pagesizelevelresult__ = 1ull << PML5_SHIFT; \
            break; \
        default: \
            verify_not_reached(); \
    } \
    __pagesizelevelresult__; \
})

enum pte_flags {
    __PTE_VALID    = 1ull << 0,
    __PTE_READ     = 1ull << 1,
    __PTE_WRITE    = 1ull << 2,
    __PTE_EXEC     = 1ull << 3,
    __PTE_USER     = 1ull << 4,
    __PTE_GLOBAL   = 1ull << 5,
    __PTE_ACCESSED = 1ull << 6,
    __PTE_DIRTY    = 1ull << 7,

    // In the "Svpbmt" RISC-V extension
    // Non-cacheable, idempotent, weakly-ordered (RVWMO), main memory
    __PTE_NC = 0b1ull << 61,

    // Non-cacheable, non-idempotent, strongly-ordered (I/O ordering), I/O
    __PTE_IO = 0b10ull << 61
};

#define PGT_FLAGS __PTE_VALID
#define PTE_LARGE_FLAGS(level) ({ (void)(level); __PTE_VALID; })
#define PTE_LEAF_FLAGS __PTE_VALID

struct page;
