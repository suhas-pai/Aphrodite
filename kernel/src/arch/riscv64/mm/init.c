/*
 * kernel/src/arch/riscv64/mm/init.c
 * © suhas pai
 */

#include "dev/printk.h"

#include "lib/align.h"
#include "lib/size.h"

#include "mm/early.h"
#include "mm/memmap.h"
#include "mm/pgmap.h"

#include "sched/process.h"
#include "sys/boot.h"

__optimize(3) static uint64_t alloc_page(void *const info) {
    (void)info;
    return early_alloc_page();
}

__optimize(3)
static uint64_t alloc_large_page(const pgt_level_t level, void *const cb_info) {
    (void)cb_info;
    return early_alloc_large_page(level);
}

__optimize(3) static void
alloc_region(const uint64_t virt_addr,
             const uint64_t map_size,
             const uint64_t pte_flags)
{
    const struct pgmap_options options = {
        .leaf_pte_flags = pte_flags,
        .large_pte_flags = options.leaf_pte_flags,

        .alloc_pgtable_cb_info = NULL,
        .free_pgtable_cb_info = NULL,

        .supports_largepage_at_level_mask = 1 << 2 | 1 << 3 | 1 << 4,

        .free_pages = false,
        .is_in_early = true,
        .is_overwrite = false
    };

    const struct pgmap_alloc_options alloc_options = {
        .alloc_page = alloc_page,
        .alloc_large_page = alloc_large_page,

        .alloc_page_cb_info = NULL,
        .alloc_large_page_cb_info = NULL,
    };

    const enum pgmap_alloc_result map_result =
        pgmap_alloc_at(&kernel_process.pagemap,
                       RANGE_INIT(virt_addr, map_size),
                       &options,
                       &alloc_options);

    switch (map_result) {
        case E_PGMAP_ALLOC_OK:
            break;
        case E_PGMAP_ALLOC_PAGE_ALLOC_FAIL:
        case E_PGMAP_ALLOC_PGTABLE_ALLOC_FAIL:
            panic("mm: ran out of free pages");
    }
}

extern uint64_t structpage_page_count;

static void setup_pagestructs_table() {
    const uint64_t table_size = structpage_page_count * SIZEOF_STRUCTPAGE;
    uint64_t map_size = table_size;

    if (!align_up(map_size, PAGE_SIZE, &map_size)) {
        panic("mm: failed to initialize memory, overflow error when aligning "
              "structpage-table size to PAGE_SIZE");
    }

    printk(LOGLEVEL_INFO,
           "mm: structpage table is %" PRIu64 " bytes\n",
           map_size);

    const uint64_t pte_flags = __PTE_READ | __PTE_WRITE | __PTE_GLOBAL;
    alloc_region(PAGE_OFFSET, map_size, pte_flags);

    PAGE_END = PAGE_OFFSET + table_size;
    printk(LOGLEVEL_INFO, "mm: finished mapping structpage-table\n");
}

static void
map_into_kernel_pagemap(const struct range phys_range,
                        const uint64_t virt_addr,
                        uint64_t pte_flags)
{
    assert_msg(pte_flags & (__PTE_READ | __PTE_WRITE | __PTE_EXEC),
               "mm: map_into_kernel_pagemap() got flags w/o any rwx "
               "permissions");

    const struct pgmap_options options = {
        .leaf_pte_flags = __PTE_GLOBAL | pte_flags,
        .large_pte_flags = options.leaf_pte_flags,

        .alloc_pgtable_cb_info = NULL,
        .free_pgtable_cb_info = NULL,

        .supports_largepage_at_level_mask =
            PAGING_MODE > 3 ? (1 << 2 | 1 << 3 | 1 << 4) : (1 << 2 | 1 << 3),

        .free_pages = false,
        .is_in_early = true,
        .is_overwrite = false,
    };

    assert_msg(
        pgmap_at(&kernel_process.pagemap, phys_range, virt_addr, &options),
        "mm: failed to setup kernel-pagemap");

    printk(LOGLEVEL_INFO,
           "mm: mapped " RANGE_FMT " to " RANGE_FMT "\n",
           RANGE_FMT_ARGS(phys_range),
           RANGE_FMT_ARGS(RANGE_INIT(virt_addr, phys_range.size)));
}

static void setup_kernel_pagemap(uint64_t *const kernel_memmap_size_out) {
    const uint64_t root_phys = early_alloc_page();
    if (root_phys == INVALID_PHYS) {
        panic("mm: failed to allocate root page for the kernel-pagemap");
    }

    kernel_process.pagemap.root = phys_to_virt(root_phys);
    map_into_kernel_pagemap(/*phys_range=*/range_create_end(kib(64), gib(4)),
                            /*virt_addr=*/kib(64),
                            __PTE_READ | __PTE_WRITE | __PTE_IO);

    // Map all 'good' regions into the hhdm
    uint64_t kernel_memmap_size = 0;
    for (uint64_t i = 0; i != mm_get_memmap_count(); i++) {
        const struct mm_memmap *const memmap = &mm_get_memmap_list()[i];
        if (memmap->kind == MM_MEMMAP_KIND_BAD_MEMORY) {
            continue;
        }

        if (memmap->kind == MM_MEMMAP_KIND_KERNEL_AND_MODULES) {
            kernel_memmap_size = memmap->range.size;
            map_into_kernel_pagemap(/*phys_range=*/memmap->range,
                                    /*virt_addr=*/KERNEL_BASE,
                                    __PTE_READ | __PTE_WRITE | __PTE_EXEC);
        } else {
            uint64_t pte_flags = __PTE_READ | __PTE_WRITE;
            map_into_kernel_pagemap(/*phys_range=*/memmap->range,
                                    (uint64_t)phys_to_virt(memmap->range.front),
                                    pte_flags);
        }
    }

    *kernel_memmap_size_out = kernel_memmap_size;

    setup_pagestructs_table();
    switch_to_pagemap(&kernel_process.pagemap);

    // All 'good' memmaps use pages with associated struct pages to them,
    // despite the pages being allocated before the structpage-table, so we have
    // to go through each table used, and setup all pgtable metadata inside a
    // struct page

    mm_early_refcount_alloced_map(kib(64), gib(4) - kib(64));
    for (uint64_t i = 0; i != mm_get_memmap_count(); i++) {
        const struct mm_memmap *const memmap = &mm_get_memmap_list()[i];
        if (memmap->kind == MM_MEMMAP_KIND_BAD_MEMORY
            || memmap->kind == MM_MEMMAP_KIND_RESERVED)
        {
            continue;
        }

        uint64_t virt_addr = 0;
        if (memmap->kind != MM_MEMMAP_KIND_KERNEL_AND_MODULES) {
            virt_addr = (uint64_t)phys_to_virt(memmap->range.front);
        } else {
            virt_addr = KERNEL_BASE;
        }

        mm_early_refcount_alloced_map(virt_addr, memmap->range.size);
    }

    printk(LOGLEVEL_INFO, "mm: finished setting up kernel-pagemap!\n");
}

static void fill_kernel_pagemap_struct(const uint64_t kernel_memmap_size) {
    // Setup vma_tree to include all ranges we've mapped.

    struct vm_area *const null_area =
        vma_alloc(&kernel_process.pagemap,
                  range_create_upto(PAGE_SIZE),
                  PROT_NONE,
                  VMA_CACHEKIND_NO_CACHE);

    // This range was never mapped, but is still reserved.
    struct vm_area *const mmio =
        vma_alloc(&kernel_process.pagemap,
                  RANGE_INIT(VMAP_BASE, (VMAP_END - VMAP_BASE)),
                  PROT_READ | PROT_WRITE,
                  VMA_CACHEKIND_MMIO);

    struct vm_area *const kernel =
        vma_alloc(&kernel_process.pagemap,
                  RANGE_INIT(KERNEL_BASE, kernel_memmap_size),
                  PROT_READ | PROT_WRITE,
                  VMA_CACHEKIND_DEFAULT);

    struct vm_area *const hhdm =
        vma_alloc(&kernel_process.pagemap,
                  RANGE_INIT(HHDM_OFFSET, gib(64)),
                  PROT_READ | PROT_WRITE,
                  VMA_CACHEKIND_DEFAULT);

    assert_msg(
        addrspace_add_node(&kernel_process.pagemap.addrspace,
                           &null_area->node)
        && addrspace_add_node(&kernel_process.pagemap.addrspace, &mmio->node)
        && addrspace_add_node(&kernel_process.pagemap.addrspace, &kernel->node)
        && addrspace_add_node(&kernel_process.pagemap.addrspace, &hhdm->node),
        "mm: failed to setup kernel-pagemap");
}

void mm_arch_init() {
    uint64_t kernel_memmap_size = 0;
    setup_kernel_pagemap(&kernel_memmap_size);

    mm_post_arch_init();
    fill_kernel_pagemap_struct(kernel_memmap_size);

    printk(LOGLEVEL_INFO, "mm: finished setting up\n");
}