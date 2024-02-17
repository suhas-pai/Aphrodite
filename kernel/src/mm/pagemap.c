/*
 * kernel/src/mm/pagemap.c
 * © suhas pai
 */

#if defined(__x86_64__)
    #include "asm/regs.h"
#elif defined(__aarch64__)
    #include "asm/ttbr.h"
    #if defined(AARCH64_USE_16K_PAGES)
        #include "asm/tcr.h"
    #endif /* defined(AARCH64_USE_16K_PAGES) */
#elif defined(__riscv64)
    #include "asm/satp.h"
#endif /* defined(__x86_64__) */

#include "cpu/info.h"
#include "sched/process.h"

#include "pgmap.h"

__optimize(3) struct pagemap pagemap_empty() {
    struct pagemap result = {
    #if defined(__aarch64__)
        .lower_root = NULL,
        .higher_root = NULL,
    #else
        .root = NULL,
    #endif /* defined(__aarch64__) */

        .addrspace = ADDRSPACE_INIT(result.addrspace),
        .addrspace_lock = SPINLOCK_INIT(),

        .cpu_list = LIST_INIT(kernel_process.pagemap.cpu_list),
        .cpu_lock = SPINLOCK_INIT(),

        .refcount = REFCOUNT_CREATE_MAX()
    };

    refcount_init(&result.refcount);
    return result;
}

#if defined(__aarch64__)
    __optimize(3) struct pagemap
    pagemap_create(pte_t *const lower_root, pte_t *const higher_root) {
        struct pagemap result = {
            .lower_root = lower_root,
            .higher_root = higher_root,

            .addrspace = ADDRSPACE_INIT(result.addrspace),

            .cpu_list = LIST_INIT(kernel_process.pagemap.cpu_list),
            .cpu_lock = SPINLOCK_INIT(),

            .addrspace_lock = SPINLOCK_INIT(),
        };

        refcount_init(&result.refcount);
        return result;
    }
#else
    __optimize(3) struct pagemap pagemap_create(pte_t *const root) {
        struct pagemap result = {
            .root = root,
            .addrspace = ADDRSPACE_INIT(result.addrspace),

            .cpu_list = LIST_INIT(kernel_process.pagemap.cpu_list),
            .cpu_lock = SPINLOCK_INIT(),

            .addrspace_lock = SPINLOCK_INIT(),
        };

        refcount_init(&result.refcount);
        return result;
    }
#endif /* defined(__aarch64__) */

bool
pagemap_find_space_and_add_vma(struct pagemap *const pagemap,
                               struct vm_area *const vma,
                               const struct range in_range,
                               const uint64_t phys_addr,
                               const uint64_t align)
{
    const int flag = spin_acquire_with_irq(&pagemap->addrspace_lock);
    const uint64_t addr =
        addrspace_find_space_and_add_node(&pagemap->addrspace,
                                          in_range,
                                          &vma->node,
                                          align);


    if (addr == ADDRSPACE_INVALID_ADDR) {
        spin_release_with_irq(&pagemap->addrspace_lock, flag);
        return false;
    }

    if (vma->prot == PROT_NONE) {
        spin_release_with_irq(&pagemap->addrspace_lock, flag);
        return true;
    }

    const int flag2 = spin_acquire_with_irq(&vma->lock);
    spin_release_with_irq(&pagemap->addrspace_lock, flag);

    const bool map_result =
        arch_make_mapping(pagemap,
                          RANGE_INIT(phys_addr, vma->node.range.size),
                          vma->node.range.front,
                          vma->prot,
                          vma->cachekind,
                          /*is_overwrite=*/false);

    spin_release_with_irq(&vma->lock, flag2);
    return map_result;
}

bool
pagemap_add_vma(struct pagemap *const pagemap,
                struct vm_area *const vma,
                uint64_t phys_addr)
{
    const int flag = spin_acquire_with_irq(&pagemap->addrspace_lock);
    if (!addrspace_add_node(&pagemap->addrspace, &vma->node)) {
        spin_release_with_irq(&pagemap->addrspace_lock, flag);
        return false;
    }

    if (vma->prot == PROT_NONE) {
        spin_release_with_irq(&pagemap->addrspace_lock, flag);
        return true;
    }

    const int flag2 = spin_acquire_with_irq(&vma->lock);
    spin_release_with_irq(&pagemap->addrspace_lock, flag);

    const bool map_result =
        arch_make_mapping(pagemap,
                          RANGE_INIT(phys_addr, vma->node.range.size),
                          vma->node.range.front,
                          vma->prot,
                          vma->cachekind,
                          /*is_overwrite=*/false);

    spin_release_with_irq(&vma->lock, flag2);
    return map_result;
}

void switch_to_pagemap(struct pagemap *const pagemap) {
#if defined(__aarch64__)
    assert(pagemap->lower_root != NULL);
    assert(pagemap->higher_root != NULL);
#else
    assert(pagemap->root != NULL);
#endif /* defined(__aarch64__) */

    const int flag = spin_acquire_with_irq(&pagemap->cpu_lock);

    list_remove(&this_cpu_mut()->pagemap_node);
    list_add(&pagemap->cpu_list, &this_cpu_mut()->pagemap_node);

#if defined(__x86_64__)
    write_cr3(virt_to_phys(pagemap->root));
#elif defined(__aarch64__)
    write_ttbr0_el1(virt_to_phys(pagemap->lower_root));
    write_ttbr1_el1(virt_to_phys(pagemap->higher_root));

    #if defined(AARCH64_USE_16K_PAGES)
        write_tcr_el1(rm_mask(read_tcr_el1(), __TCR_TG1) |
                      TCR_TG1_16KIB << TCR_TG1_SHIFT);
    #endif /* defined(AARCH64_USE_16K_PAGES) */

    asm volatile ("dsb sy; isb" ::: "memory");
#else
    const uint64_t satp =
        (SATP_MODE_39_BIT_PAGING + PAGING_MODE) << SATP_PHYS_MODE_SHIFT |
        (virt_to_phys(pagemap->root) >> PML1_SHIFT);

    write_satp(satp);
    asm volatile ("sfence.vma" ::: "memory");
#endif /* defined(__x86_64__) */

    spin_release_with_irq(&pagemap->cpu_lock, flag);
}