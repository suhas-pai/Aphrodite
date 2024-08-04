/*
 * kernel/src/arch/loongarch64/mm/types.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lib/assert.h"

#define MAX_ORDER 31

#define PML1_SHIFT 12ul
#define PML2_SHIFT 21ul
#define PML3_SHIFT 30ul
#define PML4_SHIFT 39ul

#define PGT_LEVEL_COUNT 4ul
#define PGT_PTE_COUNT(level) ({ (void)(level); (uint16_t)512; })

#define PML1_MASK 0x1ffull
#define PML2_MASK PML1_MASK
#define PML3_MASK PML1_MASK
#define PML4_MASK PML1_MASK

#define PTE_PHYS_MASK 0x0000FFFFFFFFF000
#define PTE_PHYS_LARGE_MASK 0x0000FFFFFF000000

#define PAGE_SHIFT PML1_SHIFT

#define PML1(phys) (((phys) >> PML1_SHIFT) & PML1_MASK)
#define PML2(phys) (((phys) >> PML2_SHIFT) & PML2_MASK)
#define PML3(phys) (((phys) >> PML3_SHIFT) & PML3_MASK)
#define PML4(phys) (((phys) >> PML4_SHIFT) & PML4_MASK)

#define pte_to_phys(pte, level) ({ \
    __auto_type __ptetophys_pte__ = (pte); \
    pte_level_can_have_large(level) && __ptetophys_pte__ & __PTE_LARGE ? \
        __ptetophys_pte__ & PTE_PHYS_LARGE_MASK : \
        __ptetophys_pte__ & PTE_PHYS_MASK; \
})

#define phys_create_pte(phys) ((pte_t)phys)

typedef uint64_t pte_t;
static const uint16_t PT_LEVEL_MASKS[PGT_LEVEL_COUNT + 1] = {
    (1ull << PML1_SHIFT) - 1, PML1_MASK, PML2_MASK, PML3_MASK, PML4_MASK,
};

static const uint8_t PAGE_SHIFTS[PGT_LEVEL_COUNT] = {
    PML1_SHIFT, PML2_SHIFT, PML3_SHIFT, PML4_SHIFT,
};

static const uint8_t LARGEPAGE_LEVELS[] = { 2, 3, 4 };
static const uint8_t LARGEPAGE_SHIFTS[] = {
    PML2_SHIFT, PML3_SHIFT, PML4_SHIFT
};

#define PAGE_SIZE_2MIB (1ull << PML2_SHIFT)
#define PAGE_SIZE_1GIB (1ull << PML3_SHIFT)

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
        default: \
            verify_not_reached(); \
    } \
    __pagesizelevelresult__; \
})

enum pte_shifts {
    PTE_PRIVL_SHIFT = 2,
    PTE_MEM_ACCESS_CTRL_SHIFT = 4,
};

enum pte_flags {
    __PTE_VALID  = 1ull << 0,
    __PTE_DIRTY = 1ull << 1,

    __PTE_PRIVL = 0b11 << PTE_PRIVL_SHIFT,
    __PTE_MEM_ACCESS_CTRL = 0b11 << PTE_MEM_ACCESS_CTRL_SHIFT,

    __PTE_GLOBAL = 1ull << 6,
    __PTE_LARGE = 1ull << 6,
    __PTE_WRITE = 1ull << 8,

    __PTE_GLOBAL_LARGE = 1ull << 12,

    __PTE_NO_READ = 1ull << 61,
    __PTE_NO_EXEC = 1ull << 62,
    __PTE_RESTRICT_PRIVL = 1ull << 63,
};

#define PGT_FLAGS __PTE_VALID
#define PTE_LARGE_FLAGS(level) ({ (void)(level); __PTE_VALID | __PTE_LARGE; })

#define PTE_LEAF_FLAGS __PTE_VALID

struct page;
