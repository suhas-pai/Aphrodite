/*
 * kernel/src/mm/pagemap.c
 * © suhas pai
 */

#if defined(__x86_64__)
    #include "asm/regs.h"
#elif defined(__aarch64__)
    #if defined(AARCH64_USE_16K_PAGES)
        #include "asm/tcr.h"
    #endif /* defined(AARCH64_USE_16K_PAGES) */
    #include "asm/ttbr.h"
#elif defined(__riscv64)
    #include "asm/satp.h"
#elif defined(__loongarch64)
    #include "asm/csr.h"
#endif /* defined(__x86_64__) */

#include "cpu/info.h"
#include "mm/walker.h"
#include "sched/process.h"

#include "pgmap.h"

__debug_optimize(3) struct pagemap pagemap_empty() {
    struct pagemap result = {
    #if PAGEMAP_HAS_SPLIT_ROOT
        .lower_root = NULL,
        .higher_root = NULL,
    #else
        .root = NULL,
    #endif /* PAGEMAP_HAS_SPLIT_ROOT */

        .addrspace = ADDRSPACE_INIT(result.addrspace),
        .addrspace_lock = SPINLOCK_INIT(),

        .cpu_list = LIST_INIT(kernel_process.pagemap.cpu_list),
        .cpu_lock = SPINLOCK_INIT(),
    };

    refcount_init(&result.refcount);
    return result;
}

#if PAGEMAP_HAS_SPLIT_ROOT
    __debug_optimize(3) struct pagemap
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
    __debug_optimize(3) struct pagemap pagemap_create(pte_t *const root) {
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
#endif /* PAGEMAP_HAS_SPLIT_ROOT */

bool
pagemap_find_space_and_add_vma(struct pagemap *const pagemap,
                               struct vm_area *const vma,
                               const struct range in_range,
                               const uint64_t phys_addr,
                               const uint64_t align)
{
    const int flag = spin_acquire_save_irq(&pagemap->addrspace_lock);
    const uint64_t addr =
        addrspace_find_space_and_add_node(&pagemap->addrspace,
                                          in_range,
                                          &vma->node,
                                          align);


    if (addr == ADDRSPACE_INVALID_ADDR) {
        spin_release_restore_irq(&pagemap->addrspace_lock, flag);
        return false;
    }

    if (vma->prot == PROT_NONE) {
        spin_release_restore_irq(&pagemap->addrspace_lock, flag);
        return true;
    }

    const int flag2 = spin_acquire_save_irq(&vma->lock);
    spin_release_restore_irq(&pagemap->addrspace_lock, flag);

    const bool map_result =
        arch_make_mapping(pagemap,
                          RANGE_INIT(phys_addr, vma->node.range.size),
                          vma->node.range.front,
                          vma->prot,
                          vma->cachekind,
                          /*is_overwrite=*/false);

    spin_release_restore_irq(&vma->lock, flag2);
    return map_result;
}

bool
pagemap_add_vma(struct pagemap *const pagemap,
                struct vm_area *const vma,
                uint64_t phys_addr)
{
    const int flag = spin_acquire_save_irq(&pagemap->addrspace_lock);
    if (!addrspace_add_node(&pagemap->addrspace, &vma->node)) {
        spin_release_restore_irq(&pagemap->addrspace_lock, flag);
        return false;
    }

    if (vma->prot == PROT_NONE) {
        spin_release_restore_irq(&pagemap->addrspace_lock, flag);
        return true;
    }

    const int flag2 = spin_acquire_save_irq(&vma->lock);
    spin_release_restore_irq(&pagemap->addrspace_lock, flag);

    const bool map_result =
        arch_make_mapping(pagemap,
                          RANGE_INIT(phys_addr, vma->node.range.size),
                          vma->node.range.front,
                          vma->prot,
                          vma->cachekind,
                          /*is_overwrite=*/false);

    spin_release_restore_irq(&vma->lock, flag2);
    return map_result;
}

void switch_to_pagemap(struct pagemap *const pagemap) {
#if PAGEMAP_HAS_SPLIT_ROOT
    assert(pagemap->lower_root != NULL);
    assert(pagemap->higher_root != NULL);
#else
    assert(pagemap->root != NULL);
#endif /* PAGEMAP_HAS_SPLIT_ROOT */

    const int flag = spin_acquire_save_irq(&pagemap->cpu_lock);

#if defined(__x86_64__)
    write_cr3(virt_to_phys(pagemap->root));
#elif defined(__aarch64__)
    write_ttbr0_el1(virt_to_phys(pagemap->lower_root));
    write_ttbr1_el1(virt_to_phys(pagemap->higher_root));

    #if defined(AARCH64_USE_16K_PAGES)
        write_tcr_el1(rm_mask(read_tcr_el1(), __TCR_TG1)
                    | TCR_TG1_16KIB << TCR_TG1_SHIFT);
    #endif /* defined(AARCH64_USE_16K_PAGES) */

    asm volatile ("dsb sy; isb" ::: "memory");
#elif defined(__riscv64)
    const uint64_t value =
        (SATP_MODE_39_BIT_PAGING + PAGING_MODE) << SATP_PHYS_MODE_SHIFT
      | (virt_to_phys(pagemap->root) >> PML1_SHIFT);

    csr_write(satp, value);
    asm volatile ("sfence.vma" ::: "memory");
#elif defined(__loongarch64)
    csr_write(pgdl, virt_to_phys(pagemap->lower_root));
    csr_write(pgdh, virt_to_phys(pagemap->higher_root));
#else
    verify_not_reached();
#endif /* defined(__x86_64__) */

    list_remove(&this_cpu_mut()->pagemap_node);
    list_add(&pagemap->cpu_list, &this_cpu_mut()->pagemap_node);

    spin_release_restore_irq(&pagemap->cpu_lock, flag);
}

__debug_optimize(3) uint64_t
pagemap_virt_get_phys(const struct pagemap *const pagemap, const uint64_t virt)
{
    struct pt_walker walker;
    ptwalker_create_for_pagemap(&walker, pagemap, virt, NULL, NULL);

    const uint64_t phys = ptwalker_get_phys_addr(&walker);
    if (phys == INVALID_PHYS) {
        return phys;
    }

    const uint64_t offset =
        virt & mask_for_n_bits(PAGE_SHIFTS[walker.level - 1]);

    return phys + offset;
}