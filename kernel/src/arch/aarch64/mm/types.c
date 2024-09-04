/*
 * kernel/src/arch/aarch64/mm/types.c
 * © suhas pai
 */

#include "mm/mm_types.h"

#include "limine.h"
#include "types.h"

__hidden const uint64_t PAGE_OFFSET = 0xffffc00000000000;
__hidden const uint64_t VMAP_BASE = 0xffffd00000000000;
__hidden const uint64_t VMAP_END = 0xffffe00000000000;

__hidden uint64_t PAGING_MODE = 0;
__hidden uint64_t PAGE_END = 0;

__hidden
struct largepage_level_info largepage_level_info_list[PGT_LEVEL_COUNT] = {
    [0] = {
        .order = 0,
        .largepage_order = UINT8_MAX,
        .level = 1,
        .size = PAGE_SIZE,
        .is_supported = true
    },
#if defined(AARCH64_USE_16K_PAGES)
    [LARGEPAGE_LEVEL_32MIB - 1] = {
        .order = 13,
        .largepage_order = 0,
        .level = LARGEPAGE_LEVEL_32MIB,
        .size = PAGE_SIZE_32MIB,
        .is_supported = true
    },
    [LARGEPAGE_LEVEL_64GIB - 1] = {
        .order = 24,
        .largepage_order = 1,
        .level = LARGEPAGE_LEVEL_64GIB,
        .size = PAGE_SIZE_64GIB,
        .is_supported = true
    },
    [LARGEPAGE_LEVEL_128TIB - 1] = {
        .order = 36,
        .largepage_order = 2,
        .level = LARGEPAGE_LEVEL_128TIB,
        .size = PAGE_SIZE_128TIB,
        .is_supported = true
    }
#else
    [LARGEPAGE_LEVEL_2MIB - 1] = {
        .order = 9,
        .largepage_order = 0,
        .level = LARGEPAGE_LEVEL_2MIB,
        .size = PAGE_SIZE_2MIB,
        .is_supported = true
    },
    [LARGEPAGE_LEVEL_1GIB - 1] = {
        .order = 18,
        .largepage_order = 1,
        .level = LARGEPAGE_LEVEL_1GIB,
        .size = PAGE_SIZE_1GIB,
        .is_supported = true
    },
    [LARGEPAGE_LEVEL_512GIB - 1] = {
        .order = 27,
        .largepage_order = 2,
        .level = LARGEPAGE_LEVEL_512GIB,
        .size = PAGE_SIZE_512GIB,
        .is_supported = true
    }
#endif /* defined(AARCH64_USE_16K_PAGES) */
};

__debug_optimize(3) static inline bool uses_5_level_paging() {
    return PAGING_MODE == LIMINE_PAGING_MODE_AARCH64_5LVL;
}

__debug_optimize(3) pgt_level_t pgt_get_top_level() {
    return uses_5_level_paging() ? 5 : 4;
}

__debug_optimize(3) uint64_t sign_extend_virt_addr(const uint64_t virt) {
    if (uses_5_level_paging()) {
        return sign_extend_from_index(virt, PML5_SHIFT + 3);
    }

    return sign_extend_from_index(virt, PML4_SHIFT + 8);
}

__debug_optimize(3) bool pte_is_present(const pte_t pte) {
    return pte & __PTE_VALID;
}

__debug_optimize(3) bool pte_level_can_have_large(const pgt_level_t level) {
    return level == 2 || level == 3 || level == 4;
}

__debug_optimize(3) bool pte_is_large(const pte_t pte) {
    return (pte & (__PTE_VALID | __PTE_TABLE)) == __PTE_VALID;
}

__debug_optimize(3) bool pte_is_dirty(const pte_t pte) {
    return pte & __PTE_DIRTY;
}

__debug_optimize(3) pte_t pte_read(const pte_t *const pte) {
    return *(volatile const pte_t *)pte;
}

__debug_optimize(3) void pte_write(pte_t *const pte, const pte_t value) {
    *(volatile pte_t *)pte = value;
}

__debug_optimize(3) bool
pte_flags_equal(const pte_t pte, const pgt_level_t level, const uint64_t flags)
{
    (void)level;
    const uint64_t mask =
        __PTE_VALID | __PTE_MMIO | __PTE_USER | __PTE_RO | __PTE_INNER_SH
      | __PTE_NONGLOBAL | __PTE_PXN | __PTE_UXN;

    return (pte & mask) == flags;
}