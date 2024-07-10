/*
 * kernel/src/arch/x86_64/mm/tlb.c
 * © suhas pai
 */

#include "asm/tlb.h"
#include "mm/page_alloc.h"

#include "tlb.h"

__optimize(3) static void tlb_flush_range(const struct range range) {
    range_iterate(range, addr, PAGE_SIZE) {
        invlpg(addr);
    }
}

__optimize(3) void tlb_flush_pageop(struct pageop *const pageop) {
    tlb_flush_range(pageop->flush_range);

    struct page *page = NULL;
    struct page *tmp = NULL;

    list_foreach_mut(page, tmp, &pageop->delayed_free, table.delayed_free_list)
    {
        list_deinit(&page->table.delayed_free_list);
        free_page(page);
    }
}