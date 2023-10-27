/*
 * kernel/mm/pagemap.c
 * © suhas pai
 */

#if defined(__x86_64__)
    #include "asm/regs.h"
#elif defined(__aarch64__)
    #include "asm/ttbr.h"
#endif /* defined(__x86_64__) */

#include "cpu.h"
#include "pgmap.h"

__hidden struct pagemap kernel_pagemap = {
#if defined(__aarch64__)
    .lower_root = NULL, // setup later
    .higher_root = NULL, // setup later
#else
    .root = NULL, // setup later
#endif /* defined(__aarch64__)*/

    .cpu_list = LIST_INIT(kernel_pagemap.cpu_list),
    .cpu_lock = SPINLOCK_INIT(),
    .addrspace = ADDRSPACE_INIT(kernel_pagemap.addrspace),
    .addrspace_lock = SPINLOCK_INIT(),
    .refcount = REFCOUNT_CREATE_MAX(),
};

#if defined(__aarch64__)
    struct pagemap
    pagemap_create(pte_t *const lower_root, pte_t *const higher_root) {
        struct pagemap result = {
            .lower_root = lower_root,
            .higher_root = higher_root
        };

        result.addrspace = ADDRSPACE_INIT(result.addrspace);
        refcount_init(&result.refcount);

        return result;
    }
#else
    struct pagemap pagemap_create(pte_t *const root) {
        struct pagemap result = {
            .root = root,
        };

        result.addrspace = ADDRSPACE_INIT(result.addrspace);
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
    if (!map_result) {
        return false;
    }

    return true;
}

bool
pagemap_add_vma(struct pagemap *const pagemap,
                struct vm_area *const vma,
                uint64_t phys_addr)
{
    int flag = spin_acquire_with_irq(&pagemap->addrspace_lock);
    if (!addrspace_add_node(&pagemap->addrspace, &vma->node)) {
        spin_release_with_irq(&pagemap->addrspace_lock, flag);
        return false;
    }

    spin_release_with_irq(&pagemap->addrspace_lock, flag);
    if (vma->prot == PROT_NONE) {
        return true;
    }

    flag = spin_acquire_with_irq(&vma->lock);
    const bool map_result =
        arch_make_mapping(pagemap,
                          RANGE_INIT(phys_addr, vma->node.range.size),
                          vma->node.range.front,
                          vma->prot,
                          vma->cachekind,
                          /*is_overwrite=*/false);

    spin_release_with_irq(&vma->lock, flag);
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

    list_remove(&get_cpu_info_mut()->pagemap_node);
    list_add(&pagemap->cpu_list, &get_cpu_info_mut()->pagemap_node);

    get_cpu_info_mut()->pagemap = pagemap;

#if defined(__x86_64__)
    write_cr3(virt_to_phys(pagemap->root));
#elif defined(__aarch64__)
    write_ttbr0_el1(virt_to_phys(pagemap->lower_root));
    write_ttbr1_el1(virt_to_phys(pagemap->higher_root));

    asm volatile ("dsb sy; isb" ::: "memory");
#else
    const uint64_t satp =
        (PAGING_MODE + 8) << 60 | (virt_to_phys(pagemap->root) >> PML1_SHIFT);

    asm volatile ("csrw satp, %0; sfence.vma" :: "r"(satp) : "memory");
#endif /* defined(__x86_64__) */

    spin_release_with_irq(&pagemap->cpu_lock, flag);
}