/*
 * kernel/src/mm/simple_alloc.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/align.h"

#include "mm/page_alloc.h"
#include "mm/simple_alloc.h"

struct simple_page {
    void *virt;

    uint16_t index;
    uint32_t refcount;
};

#define ALIGNMENT 16

__debug_optimize(3) void simple_alloc_create(struct simple_alloc *const alloc) {
    alloc->page_list = ARRAY_INIT(sizeof(struct simple_page));
}

void simple_alloc_destroy(struct simple_alloc *const alloc) {
    array_foreach(&alloc->page_list, struct simple_page, page) {
        page->refcount--;
        if (page->refcount == 0) {
            printk(LOGLEVEL_WARN,
                   "mm: leaks detected in page %p, count: %d\n",
                   virt_to_page(page->virt),
                   page->refcount);
        }

        free_page(virt_to_page(page->virt));
    }
}

void *add_new_page(struct simple_alloc *const alloc, const uint32_t size) {
    struct page *const page = alloc_page(PAGE_STATE_USED, __ALLOC_ZERO);
    if (page == nullptr) {
        return nullptr;
    }

    const struct simple_page simple_page = {
        .virt = page_to_virt(page),
        .index = size,
        .refcount = 1,
    };

    if (!array_append(&alloc->page_list, &simple_page)) {
        free_page(page);
        return nullptr;
    }

    return simple_page.virt;
}

void *simple_alloc(struct simple_alloc *const alloc, const uint32_t bad_size) {
    const uint32_t size = align_up_assert(bad_size, ALIGNMENT);
    if (__builtin_expect(size > PAGE_SIZE, 0)) {
        return nullptr;
    }

    if (__builtin_expect(array_empty(alloc->page_list), 0)) {
        return add_new_page(alloc, size);
    }

    struct simple_page *const page = array_back(alloc->page_list);
    if (page->index + size >= PAGE_SIZE) {
        return add_new_page(alloc, size);
    }

    void *const result = page->virt + page->index;

    page->index += size;
    page->refcount++;

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
    array_foreach(&alloc->page_list, struct simple_page, page) {
        const struct range page_range =
            RANGE_INIT((uint64_t)page->virt, PAGE_SIZE);

        if (range_has_loc(page_range, (uint64_t)buffer)) {
            if (page->refcount == 0) {
                printk(LOGLEVEL_WARN,
                       "mm: double free detected in page %p, for allocator %p, "
                       "caller alloc: %p\n",
                       virt_to_page(page->virt),
                       alloc,
                       buffer);
                return true;
            }

            page->refcount--;
            if (page->refcount == 0 && array_item_count(alloc->page_list) > 1) {
                free_page(virt_to_page(page->virt));
                array_remove_index(&alloc->page_list,
                                   array_indexof(alloc->page_list, page));
            }

            return true;
        }
    }

    return false;
}

void simple_free(struct simple_alloc *const alloc, void *const buffer) {;
    assert(simple_try_free(alloc, buffer));
}
