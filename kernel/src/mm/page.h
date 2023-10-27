/*
 * kernel/mm/page.h
 * © suhas pai
 */

#pragma once
#include "lib/refcount.h"

#include "mm/section.h"
#include "mm/slab.h"

#include "mm_types.h"

enum page_state {
    PAGE_STATE_FREE_LIST_TAIL,
    PAGE_STATE_FREE_LIST_HEAD,

    PAGE_STATE_SYSTEM_CRUCIAL,

    PAGE_STATE_LRU_CACHE,
    PAGE_STATE_SLAB_HEAD,
    PAGE_STATE_SLAB_TAIL,
    PAGE_STATE_TABLE,
    PAGE_STATE_LARGE_HEAD,
    PAGE_STATE_LARGE_TAIL,

    PAGE_STATE_USED,
};

typedef uint8_t page_section_t;

struct page {
    _Atomic uint32_t flags;
    _Atomic uint8_t state;

    page_section_t section;
    uint16_t extra;

    union {
        struct {
            struct list freelist;
            uint8_t order;
        } freelist_head;
        struct {
            struct page *head;
        } freelist_tail;
        struct {
            union {
                struct list lru;
                struct page *head;
            };

            // An amount of 0 means this is a tail-page of a page in the lru
            // cache.
            uint32_t amount;
        } dirty_lru;
        struct {
            struct slab_allocator *allocator;
            union {
                struct {
                    struct list slab_list;

                    uint32_t free_obj_count;
                    uint32_t first_free_index;
                } head;
                struct {
                    struct page *head;
                } tail;
            };
        } slab;
        struct {
            struct refcount refcount;
            struct list delayed_free_list;
        } table;
        struct {
            struct refcount page_refcount;
            struct list delayed_free_list;

            struct refcount refcount;
            pgt_level_t level;
        } largehead;
        struct {
            struct refcount refcount;
            struct page *head;
        } largetail;
        struct {
            struct refcount refcount;
            struct list delayed_free_list;
        } used;
    };
};

_Static_assert(sizeof(struct page) == SIZEOF_STRUCTPAGE,
               "SIZEOF_STRUCTPAGE is incorrect");

enum struct_page_flags {
    PAGE_IS_DIRTY = 1 << 0,
};

uint32_t page_get_flags(const struct page *page);

void page_set_flag(struct page *page, enum struct_page_flags flag);
bool page_has_flag(const struct page *page, enum struct_page_flags flag);
void page_clear_flag(struct page *page, enum struct_page_flags flag);

enum page_state page_get_state(const struct page *page);
void page_set_state(struct page *page, enum page_state state);

struct page_section *page_to_section(const struct page *page);