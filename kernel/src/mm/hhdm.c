/*
 * kernel/src/mm/hhdm.c
 * © suhas pai
 */

#include "lib/overflow.h"
#include "mm/mm_types.h"

__hidden uint64_t HHDM_OFFSET = 0;

__debug_optimize(3) void *phys_to_virt(const uint64_t phys) {
    assert_msg(phys >= PAGE_SIZE, "phys_to_virt() got phys %p", (void *)phys);
    return (void *)check_add_assert(HHDM_OFFSET, phys);
}

__debug_optimize(3) uint64_t virt_to_phys(volatile const void *const virt) {
    assert_msg((uint64_t)virt >= HHDM_OFFSET, "virt_to_phys() got %p", virt);
    return (uint64_t)virt - HHDM_OFFSET;
}