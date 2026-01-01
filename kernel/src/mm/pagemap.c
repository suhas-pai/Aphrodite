/*
 * kernel/src/mm/pagemap.c
 * © suhas pai
 */

#include "cpu/info.h"

#include "mm/walker.h"
#include "mm/pgmap.h"
#include "mm/switch.h"

#include "sched/process.h"

__debug_optimize(3) struct pagemap pagemap_empty() {
    struct pagemap result = {
    #if PGT_HAS_SPLIT_ROOT
        .lower_root = nullptr,
        .higher_root = nullptr,
    #else
        .root = nullptr,
    #endif /* PGT_HAS_SPLIT_ROOT */

        .addrspace = ADDRSPACE_INIT(result.addrspace),
        .addrspace_lock = SPINLOCK_INIT(),

        .cpu_list = LIST_INIT(result.cpu_list),
        .cpu_lock = SPINLOCK_INIT(),
    };

    refcount_init(&result.refcount);
    return result;
}

#if PGT_HAS_SPLIT_ROOT
    __debug_optimize(3) struct pagemap
    pagemap_create(pte_t *const lower_root, pte_t *const higher_root) {
        struct pagemap result = {
            .lower_root = lower_root,
            .higher_root = higher_root,

            .addrspace = ADDRSPACE_INIT(result.addrspace),

            .cpu_list = LIST_INIT(result.cpu_list),
            .cpu_lock = SPINLOCK_INIT(),

            .addrspace_lock = SPINLOCK_INIT(),
        };

        refcount_init(&result.refcount);
        return result;
    }
#else
    __debug_optimize(3) struct pagemap pagemap_create(pte_t *const root) {
        struct pagemap result = {
            .root = root,
            .addrspace = ADDRSPACE_INIT(result.addrspace),

            .cpu_list = LIST_INIT(result.cpu_list),
            .cpu_lock = SPINLOCK_INIT(),

            .addrspace_lock = SPINLOCK_INIT(),
        };

        refcount_init(&result.refcount);
        return result;
    }
#endif /* PGT_HAS_SPLIT_ROOT */

bool
pagemap_find_space_and_add_vma(struct pagemap *const pagemap,
                               struct vm_area *const vma,
                               const struct range in_range,
                               const uint64_t phys_addr,
                               const uint64_t align)
{
    const int flag = spin_acquire_save_intr(&pagemap->addrspace_lock);
    const uint64_t addr =
        addrspace_find_space_and_add_node(&pagemap->addrspace,
                                          in_range,
                                          &vma->node,
                                          align);


    if (addr == ADDRSPACE_INVALID_ADDR) {
        spin_release_restore_intr(&pagemap->addrspace_lock, flag);
        return false;
    }

    if (vma->prot == PROT_NONE) {
        spin_release_restore_intr(&pagemap->addrspace_lock, flag);
        return true;
    }

    const int flag2 = spin_acquire_save_intr(&vma->lock);
    spin_release_restore_intr(&pagemap->addrspace_lock, flag);

    const bool map_result =
        arch_make_mapping(pagemap,
                          RANGE_INIT(phys_addr, vma->node.range.size),
                          vma->node.range.front,
                          vma->prot,
                          vma->cachekind,
                          /*is_overwrite=*/false);

    spin_release_restore_intr(&vma->lock, flag2);
    return map_result;
}

bool
pagemap_add_vma(struct pagemap *const pagemap,
                struct vm_area *const vma,
                uint64_t phys_addr)
{
    const int flag = spin_acquire_save_intr(&pagemap->addrspace_lock);
    if (!addrspace_add_node(&pagemap->addrspace, &vma->node)) {
        spin_release_restore_intr(&pagemap->addrspace_lock, flag);
        return false;
    }

    if (vma->prot == PROT_NONE) {
        spin_release_restore_intr(&pagemap->addrspace_lock, flag);
        return true;
    }

    const int flag2 = spin_acquire_save_intr(&vma->lock);
    spin_release_restore_intr(&pagemap->addrspace_lock, flag);

    const bool map_result =
        arch_make_mapping(pagemap,
                          RANGE_INIT(phys_addr, vma->node.range.size),
                          vma->node.range.front,
                          vma->prot,
                          vma->cachekind,
                          /*is_overwrite=*/false);

    spin_release_restore_intr(&vma->lock, flag2);
    return map_result;
}

void switch_to_pagemap(struct pagemap *const pagemap) {
#if PGT_HAS_SPLIT_ROOT
    assert(pagemap->lower_root != nullptr);
    assert(pagemap->higher_root != nullptr);
#else
    assert(pagemap->root != nullptr);
#endif /* PGT_HAS_SPLIT_ROOT */

    with_spinlock_intr_disabled(&pagemap->cpu_lock, {
    #if PGT_HAS_SPLIT_ROOT
        mm_switch_root(virt_to_phys(pagemap->lower_root),
                       virt_to_phys(pagemap->higher_root));
    #else
        mm_switch_root(virt_to_phys(pagemap->root));
    #endif /* PGT_HAS_SPLIT_ROOT */

        list_remove(&this_cpu_mut()->pagemap_node);
        list_add(&pagemap->cpu_list, &this_cpu_mut()->pagemap_node);
    });
}

__debug_optimize(3) uint64_t
pagemap_virt_get_phys(const struct pagemap *const pagemap, const uint64_t virt)
{
    struct pg_walker walker;
    pgwalker_create_for_pagemap(&walker,
                                pagemap,
                                virt,
                                /*alloc_pgtable=*/nullptr,
                                /*free_pgtable=*/nullptr);

    const uint64_t phys = pgwalker_get_phys_addr(&walker);
    if (phys == INVALID_PHYS) {
        return phys;
    }

    const uint64_t offset =
        virt & mask_for_n_bits(PAGE_SHIFTS[walker.level - 1]);

    return phys + offset;
}