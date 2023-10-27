/*
 * kernel/mm/hhdm.c
 * © suhas pai
 */

#include "lib/assert.h"
#include "lib/overflow.h"

#include "limine.h"

__hidden uint64_t HHDM_OFFSET = 0;

__optimize(3) void *phys_to_virt(const uint64_t phys) {
    return (void *)check_add_assert(HHDM_OFFSET, phys);
}

__optimize(3) uint64_t virt_to_phys(volatile const void *const virt) {
    assert_msg((uint64_t)virt >= HHDM_OFFSET, "virt_to_phys(): got %p\n", virt);
    return (uint64_t)virt - HHDM_OFFSET;
}