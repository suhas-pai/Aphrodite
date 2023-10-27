/*
 * kernel/mm/pageop.c
 * © suhas pai
 */

#include "dev/printk.h"

#if __has_include("mm/tlb.h")
    #include "mm/tlb.h"
#endif /* __has_include("mm/tlb.h") */

#include "cpu.h"

#include "pagemap.h"
#include "page_alloc.h"

#include "walker.h"

__optimize(3) void
pageop_init(struct pageop *const pageop,
            struct pagemap *const pagemap,
            const struct range range)
{
    pageop->pagemap = pagemap;
    pageop->flush_range = range;

    list_init(&pageop->delayed_free);
}

void
pageop_flush_pte_in_current_range(struct pageop *const pageop,
                                  const pte_t pte,
                                  const pgt_level_t level,
                                  const bool should_free_pages)
{
    const uint64_t pte_phys = pte_to_phys(pte);
    struct page *const pte_page = phys_to_page(pte_phys);

    if (page_get_state(pte_page) != PAGE_STATE_TABLE) {
        deref_page(pte_page, pageop);
        return;
    }

    struct pt_walker walker;
    ptwalker_create_from_toplevel(&walker,
                                  pte_phys,
                                  level,
                                  /*root_index=*/0,
                                  /*alloc_pgtable=*/NULL,
                                  /*free_pgtable=*/NULL);

    while (true) {
        pte_t *const walker_pte =
            walker.tables[walker.level - 1] + walker.indices[walker.level - 1];

        const pte_t entry = pte_read(walker_pte);
        struct page *const page = pte_to_page(entry);

        if (pte_is_large(entry)) {
            if (ref_down(&page->largehead.refcount) && should_free_pages) {
                list_add(&pageop->delayed_free,
                         &page->largehead.delayed_free_list);
            }
        } else {
            if (ref_down(&page->used.refcount) && should_free_pages) {
                list_add(&pageop->delayed_free, &page->used.delayed_free_list);
            }
        }

        ptwalker_deref_from_level(&walker, walker.level, pageop);

        const enum pt_walker_result result = ptwalker_next(&walker);
        if (__builtin_expect(result == E_PT_WALKER_OK, 1)) {
            continue;
        }

        if (result == E_PT_WALKER_REACHED_END) {
            break;
        }

        panic("mm: got result=%d while flushing pte\n", result);
    }
}

void
pageop_setup_for_address(struct pageop *const pageop, const uint64_t virt) {
    if (virt + PAGE_SIZE == pageop->flush_range.front) {
        pageop->flush_range.front = virt;
        return;
    }

    if (range_has_loc(pageop->flush_range, virt)) {
        return;
    }

    uint64_t end = 0;
    if (range_get_end(pageop->flush_range, &end)) {
        if (virt == end) {
            pageop->flush_range.size += PAGE_SIZE;
            return;
        }
    }

    pageop_finish(pageop);
    pageop->flush_range = RANGE_INIT(virt, PAGE_SIZE);
}

void
pageop_setup_for_range(struct pageop *const pageop, const struct range virt) {
    if (range_has(pageop->flush_range, virt)) {
        return;
    }

    uint64_t virt_end = 0;
    if (range_get_end(virt, &virt_end)) {
        if (pageop->flush_range.front == virt_end) {
            pageop->flush_range.front = virt.front;
            return;
        }
    }

    uint64_t flush_end = 0;
    if (range_get_end(pageop->flush_range, &flush_end)) {
        if (virt.front == flush_end) {
            pageop->flush_range.size += virt.size;
            return;
        }
    }

    pageop_finish(pageop);
    pageop->flush_range = virt;
}

__optimize(3) static void free_all_pages(struct pageop *const pageop) {
    struct page *iter = NULL;
    list_foreach(iter, &pageop->delayed_free, table.delayed_free_list) {
        free_page(iter);
    }
}

__optimize(3) void pageop_finish(struct pageop *const pageop) {
    if (range_empty(pageop->flush_range)) {
        free_all_pages(pageop);
        return;
    }

    if (get_cpu_info()->pagemap == pageop->pagemap) {
    #if defined(__x86_64__)
        tlb_flush_pageop(pageop);
    #elif defined(__riscv64)
        asm volatile ("fence.i" ::: "memory");
    #endif /* defined(__x86_64__) */
    }

    free_all_pages(pageop);
    pageop->flush_range = RANGE_EMPTY();
}