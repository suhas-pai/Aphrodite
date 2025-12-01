/*
 * kernel/src/mm/simple_alloc.c
 * © suhas pai
 */

#include <lib/align.h>
#include <lib/util.h>

#include "dev/printk.h"

#include "mm/page_alloc.h"
#include "mm/simple_alloc.h"

#define ALIGNMENT 16

__debug_optimize(3)
bool simple_alloc_initialized(const struct simple_alloc *const alloc) {
    return alloc->page_list.next != nullptr;
}

__debug_optimize(3) void simple_alloc_init(struct simple_alloc *const alloc) {
    list_init(&alloc->page_list);
}

void simple_alloc_destroy(struct simple_alloc *const alloc) {
    struct page *page = nullptr;
    list_foreach(&alloc->page_list, simple_alloc.list, page) {
        if (page->simple_alloc.refcount != 0) {
            printk(LOGLEVEL_CRITICAL,
                   "mm: leaks detected in simple_alloc page %p, "
                   "count: %" PRIu32 "\n",
                   page,
                   page->simple_alloc.refcount);
        }

        free_page(page);
    }
}

void *add_new_page(struct simple_alloc *const alloc, const uint32_t size) {
    struct page *const page = alloc_page(PAGE_STATE_SIMPLE_ALLOC, __ALLOC_ZERO);
    if (page == nullptr) {
        return nullptr;
    }

    list_radd(&alloc->page_list, &page->simple_alloc.list);

    page->simple_alloc.allocator = alloc;
    page->simple_alloc.index += size;

    return page_to_virt(page);
}

void *simple_alloc(struct simple_alloc *const alloc, const uint32_t bad_size) {
    const uint32_t size = align_up_assert(bad_size, ALIGNMENT);
    if (__builtin_expect(size > PAGE_SIZE, 0)) {
        return nullptr;
    }

    if (__builtin_expect(list_empty(&alloc->page_list), 0)) {
        return add_new_page(alloc, size);
    }

    struct page *page =
        list_tail(&alloc->page_list, struct page, simple_alloc.list);

    if (!index_in_bounds(page->simple_alloc.index + size, PAGE_SIZE)) {
        struct page *search_page = nullptr;
        bool found = false;

        list_foreach(&alloc->page_list, simple_alloc.list, search_page) {
            if (search_page->simple_alloc.index + size <= PAGE_SIZE) {
                found = true;
                break;
            }
        }

        if (!found) {
            return add_new_page(alloc, size);
        }

        page = search_page;
    }

    void *const result = page_to_virt(page) + page->simple_alloc.index;

    page->simple_alloc.index += size;
    page->simple_alloc.refcount++;

    return result;
}

void *
simple_alloc_size(struct simple_alloc *const alloc,
                  const uint32_t size,
                  uint32_t *const size_out)
{
    *size_out = align_up_assert(size, ALIGNMENT);
    return simple_alloc(alloc, size);
}

bool simple_try_free(struct simple_alloc *const alloc, void *const buffer) {
    struct page *page = nullptr;
    uint8_t count = 0;

    list_foreach(&alloc->page_list, simple_alloc.list, page) {
        count++;
        if (count > 1) {
            break;
        }
    }

    struct page *tmp = nullptr;
    list_foreach_mut(&alloc->page_list, simple_alloc.list, page, tmp) {
        const struct range page_range =
            RANGE_INIT((uint64_t)page_to_virt(page), PAGE_SIZE);

        if (!range_has_loc(page_range, (uint64_t)buffer)) {
            continue;
        }

        if (page->simple_alloc.refcount == 0) {
            printk(LOGLEVEL_WARN,
                   "mm: double free detected in page %p, for allocator %p, "
                   "caller alloc: %p\n",
                   (void *)page_range.front,
                   alloc,
                   buffer);
            return true;
        }

        page->simple_alloc.refcount--;
        if (page->simple_alloc.refcount == 0 && count > 1) {
            list_remove(&page->simple_alloc.list);
            free_page(page);
        }

        return true;
    }

    return false;
}

void simple_free(struct simple_alloc *const alloc, void *const buffer) {;
    assert(simple_try_free(alloc, buffer));
}
