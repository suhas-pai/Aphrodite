/*
 * kernel/arch/x86_64/mm/types.c
 * © suhas pai
 */

#include "lib/macros.h"

#include "cpu.h"
#include "limine.h"

__hidden const uint64_t PAGE_OFFSET = 0xffffc00000000000;
__hidden const uint64_t VMAP_BASE = 0xffffd00000000000;
__hidden const uint64_t VMAP_END = 0xffffe00000000000;

__hidden uint64_t PAGING_MODE = 0;
__hidden uint64_t PAGE_END = 0;

struct largepage_level_info largepage_level_info_list[PGT_LEVEL_COUNT] = {
    [LARGEPAGE_LEVEL_2MIB] = {
        .order = 9,
        .largepage_order = 0,
        .level = LARGEPAGE_LEVEL_2MIB,
        .size = PAGE_SIZE_2MIB,
        .is_supported = true
    },
    [LARGEPAGE_LEVEL_1GIB] = {
        .order = 18,
        .largepage_order = 1,
        .level = LARGEPAGE_LEVEL_1GIB,
        .size = PAGE_SIZE_1GIB,
        .is_supported = false
    }
};

__optimize(3) pgt_level_t pgt_get_top_level() {
    const bool has_5lvl_paging = PAGING_MODE == LIMINE_PAGING_MODE_X86_64_5LVL;
    return has_5lvl_paging ? 5 : 4;
}

__optimize(3) bool pte_is_present(const pte_t pte) {
    return (pte & __PTE_PRESENT) != 0;
}

__optimize(3) bool pte_level_can_have_large(const pgt_level_t level) {
    return largepage_level_info_list[level].is_supported;
}

__optimize(3) bool pte_is_large(const pte_t pte) {
    return (pte & __PTE_LARGE) != 0;
}

__optimize(3) bool pte_is_dirty(const pte_t pte) {
    return (pte & __PTE_DIRTY) != 0;
}

__optimize(3) pte_t pte_read(const pte_t *const pte) {
    return *pte;
}

__optimize(3) void pte_write(pte_t *const pte, const pte_t value) {
    *pte = value;
}

__optimize(3)
bool pte_flags_equal(pte_t pte, const pgt_level_t level, const uint64_t flags) {
    (void)level;
    const uint64_t mask =
        __PTE_PRESENT | __PTE_WRITE | __PTE_USER | __PTE_PWT | __PTE_PCD |
        __PTE_GLOBAL | __PTE_NOEXEC;

    return (pte & mask) == flags;
}