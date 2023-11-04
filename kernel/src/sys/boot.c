/*
 * sys/boot.c
 * © suhas pai
 */

#include "dev/printk.h"

#include "mm/memmap.h"
#include "mm/mm_types.h"
#include "mm/section.h"

#include "time/kstrftime.h"

#include "boot.h"
#include "limine.h"

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, in C, they should
// NOT be made "static".

struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
    .response = NULL
};

struct limine_kernel_address_request kern_addr_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0,
    .response = NULL
};

struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = NULL
};

struct limine_paging_mode_request paging_mode_request = {
    .id = LIMINE_PAGING_MODE_REQUEST,
    .revision = 0,
    .response = NULL
};

struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

struct limine_dtb_request dtb_request = {
    .id = LIMINE_DTB_REQUEST,
    .revision = 0,
    .response = NULL
};

struct limine_boot_time_request boot_time_request = {
    .id = LIMINE_BOOT_TIME_REQUEST,
    .revision = 0
};

static struct mm_memmap mm_memmap_list[255] = {0};
static uint8_t mm_memmap_count = 0;

// If we have a memmap whose memory pages will go in the structpage table, then
// they will also be stored in the mm_usable_list array.
//
// We store twice so the functions `phys_to_pfn()` can efficiently find the
// memmap that corresponds to a physical address w/o having to traverse the
// entire memmap-list, which is made up mostly of memmaps not mapped into the
// structpage-table

static struct page_section mm_page_section_list[255] = {0};
static uint8_t mm_page_section_count = 0;

static const void *rsdp = NULL;
static const void *dtb = NULL;

static int64_t boot_time = 0;

__optimize(3) const struct mm_memmap *mm_get_memmap_list() {
    return mm_memmap_list;
}

__optimize(3) struct page_section *mm_get_page_section_list() {
    return mm_page_section_list;
}

__optimize(3) uint8_t mm_get_memmap_count() {
    return mm_memmap_count;
}

__optimize(3) uint8_t mm_get_section_count() {
    return mm_page_section_count;
}

__optimize(3) const void *boot_get_rsdp() {
    return rsdp;
}

__optimize(3) const void *boot_get_dtb() {
    return dtb;
}

__optimize(3) int64_t boot_get_time() {
    return boot_time;
}

__optimize(3) uint64_t mm_get_full_section_mask() {
    return (1ull << mm_page_section_count) - 1;
}

__optimize(3) void boot_init() {
    assert(hhdm_request.response != NULL);
    assert(kern_addr_request.response != NULL);
    assert(memmap_request.response != NULL);
    assert(paging_mode_request.response != NULL);

    HHDM_OFFSET = hhdm_request.response->offset;
    KERNEL_BASE = kern_addr_request.response->virtual_base;
    PAGING_MODE = paging_mode_request.response->mode;

    const struct limine_memmap_response *const resp = memmap_request.response;

    struct limine_memmap_entry *const *entries = resp->entries;
    struct limine_memmap_entry *const *const end = &entries[resp->entry_count];

    uint8_t memmap_index = 0;
    uint8_t usable_index = 0;

    // Usable (and bootloader-reclaimable) memmaps are sparsely located in the
    // physical memory-space, but are stored together (contiguously) in the
    // structpage-table in virtual-memory.
    // To accomplish this, assign each memmap placed in the structpage-table a
    // pfn that will be assigned to the first page residing in the memmap.

    uint64_t pfn = 0;
    for (__auto_type memmap_iter = entries; memmap_iter != end; memmap_iter++) {
        if (memmap_index == countof(mm_memmap_list)) {
            panic("boot: too many memmaps\n");
        }

        const struct limine_memmap_entry *const memmap = *memmap_iter;
        struct range range = RANGE_EMPTY();

        if (!range_align_out(RANGE_INIT(memmap->base, memmap->length),
                             /*boundary=*/PAGE_SIZE,
                             &range))
        {
            panic("boot: failed to align memmap");
        }

        // If we find overlapping memmaps, try and fix the range of our current
        // memmap based on the range of the previous memmap. We assume the
        // memory-maps are sorted in ascending order.

        if (memmap_index != 0) {
            const struct range prev_range =
                mm_memmap_list[memmap_index - 1].range;

            if (range_overlaps(prev_range, range)) {
                if (range_has(prev_range, range)) {
                    // If the previous memmap completely contains this memmap,
                    // then simply skip this memmap.

                    continue;
                }

                const uint64_t prev_end = range_get_end_assert(prev_range);
                range = range_from_loc(range, prev_end);
            }
        }

        mm_memmap_list[memmap_index] = (struct mm_memmap){
            .range = range,
            .kind = (enum mm_memmap_kind)(memmap->type + 1),
        };

        if (memmap->type == LIMINE_MEMMAP_USABLE) {
            struct page_section *const section =
                &mm_page_section_list[usable_index];

            page_section_init(section,
                              /*zone=*/NULL,
                              mm_memmap_list[memmap_index].range,
                              pfn);

            for (uint8_t i = 0; i != MAX_ORDER; i++) {
                list_init(&section->freelist_list[i].page_list);
                section->freelist_list[i].count = 0;
            }

            pfn += PAGE_COUNT(range.size);
            usable_index++;
        }

        memmap_index++;
    }

    mm_memmap_count = memmap_index;
    mm_page_section_count = usable_index;
}

void boot_post_early_init() {
    printk(LOGLEVEL_INFO,
           "boot: there are %" PRIu8 " usable memmaps\n",
           mm_page_section_count);

    if (dtb_request.response == NULL || dtb_request.response->dtb_ptr == NULL) {
        printk(LOGLEVEL_WARN, "boot: device tree is missing\n");
    } else {
        dtb = dtb_request.response->dtb_ptr;
    }

    if (rsdp_request.response == NULL ||
        rsdp_request.response->address == NULL)
    {
    #if !defined(__riscv64)
        panic("boot: acpi not found\n");
    #else
        printk(LOGLEVEL_WARN, "boot: acpi does not exist\n");
    #endif /* !defined(__riscv64) */
    } else {
        rsdp = rsdp_request.response->address;
    }

    if (boot_time_request.response == NULL) {
        panic("boot: boot-time not found\n");
    }

    boot_time = boot_time_request.response->boot_time;
    printk(LOGLEVEL_INFO, "boot: boot timestamp is %" PRIu64 ": ", boot_time);

    struct tm tm = tm_from_stamp((uint64_t)boot_time);
    printk_strftime("%c\n", &tm);
}

__optimize(3) void boot_merge_usable_memmaps() {
    for (uint64_t index = 1; index != mm_page_section_count; index++) {
        struct page_section *const memmap = &mm_page_section_list[index];
        do {
            struct page_section *const prev_memmap = memmap - 1;
            const uint64_t prev_end = range_get_end_assert(prev_memmap->range);

            if (prev_end != memmap->range.front) {
                break;
            }

            prev_memmap->range.size += memmap->range.size;

            // Remove the current memmap.
            const uint64_t move_amount =
                (mm_page_section_count - (index + 1)) *
                sizeof(struct page_section);

            memmove(memmap, memmap + 1, move_amount);
            mm_page_section_count--;

            if (index == mm_page_section_count) {
                return;
            }
        } while (true);
    }
}

__optimize(3) void boot_remove_section(struct page_section *const section) {
    const uint64_t length =
        distance(section + 1, &mm_page_section_list[mm_page_section_count]);

    memmove(section, section + 1, length);
}

__optimize(3)
struct page_section *boot_add_section_at(struct page_section *const section) {
    const uint64_t length =
        distance(section, &mm_page_section_list[mm_page_section_count]);

    memmove(section + 1, section, length);
    mm_page_section_count++;

    return section;
}

__optimize(3) void boot_recalculate_pfns() {
    struct page_section *section = mm_page_section_list;
    const struct page_section *const end =
        &mm_page_section_list[mm_page_section_count];

    for (uint64_t pfn = 0; section != end; section++) {
        section->pfn = pfn;
        pfn += PAGE_COUNT(section->range.size);
    }
}