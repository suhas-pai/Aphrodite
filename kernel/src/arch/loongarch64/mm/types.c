/*
 * kernel/src/arch/loongarch64/mm/types.c
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
};

__debug_optimize(3) pgt_level_t pgt_get_top_level() {
    return 4;
}

__debug_optimize(3) uint64_t sign_extend_virt_addr(const uint64_t virt) {
    return sign_extend_from_index(virt, PML4_SHIFT + 8);
}

__debug_optimize(3) bool pte_is_present(const pte_t pte) {
    return pte & __PTE_VALID;
}

__debug_optimize(3) bool pte_level_can_have_large(const pgt_level_t level) {
    return level == 2 || level == 3;
}

__debug_optimize(3) bool pte_is_large(const pte_t pte) {
    return pte & __PTE_LARGE;
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
    const uint64_t global_mask =
        pte_level_can_have_large(level) && pte_is_large(pte) ?
            __PTE_GLOBAL_LARGE : __PTE_GLOBAL;

    const uint64_t mask =
        __PTE_VALID | __PTE_PRIVL | __PTE_MEM_ACCESS_CTRL | global_mask
      | __PTE_WRITE | __PTE_NO_READ | __PTE_NO_EXEC;

    return (pte & mask) == flags;
}
