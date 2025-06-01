/*
 * kernel/src/sys/boot.c
 * © suhas pai
 */

#include "mm/memmap.h"
#include "mm/mm_types.h"
#include "mm/section.h"

#include "time/kstrftime.h"
#include "limine.h"

#define MM_MAX_SECTION_COUNT sizeof_bits(uint64_t)

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent.

__attribute__((section(".requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = nullptr
};

__attribute__((section(".requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
    .response = nullptr
};

__attribute__((section(".requests")))
static volatile struct limine_executable_address_request exec_addr_request = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST,
    .revision = 0,
    .response = nullptr
};

__attribute__((section(".requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = nullptr
};

__attribute__((section(".requests")))
static volatile struct limine_paging_mode_request paging_mode_request = {
    .id = LIMINE_PAGING_MODE_REQUEST,
    .revision = 0,
    .response = nullptr
};

__attribute__((section(".requests")))
static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0,
    .response = nullptr,
};

__attribute__((section(".requests")))
static volatile struct limine_dtb_request dtb_request = {
    .id = LIMINE_DTB_REQUEST,
    .revision = 0,
    .response = nullptr
};

__attribute__((section(".requests")))
static volatile struct limine_date_at_boot_request boot_time_request = {
    .id = LIMINE_DATE_AT_BOOT_REQUEST,
    .revision = 0,
    .response = nullptr,
};

__attribute__((section(".requests")))
static volatile struct limine_mp_request mp_request = {
    .id = LIMINE_MP_REQUEST,
    .revision = 0,
    .response = nullptr,
#if defined(__x86_64__)
    .flags = LIMINE_MP_X2APIC,
#else
    .flags = 0,
#endif /* defined(__x86_64__) */
};

static uint64_t g_slide = 0;

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".requests_start_marker")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".requests_end_marker")))
static volatile LIMINE_REQUESTS_END_MARKER;

static struct limine_framebuffer_response framebuffer_resp = {0};
static struct limine_mp_response *mp_response = nullptr;

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

static const void *rsdp = nullptr;
static const void *dtb = nullptr;

static uint64_t boot_time = 0;

__debug_optimize(3) const struct mm_memmap *mm_get_memmap_list() {
    return mm_memmap_list;
}

__debug_optimize(3) struct page_section *mm_get_page_section_list() {
    return mm_page_section_list;
}

__debug_optimize(3) uint8_t mm_get_memmap_count() {
    return mm_memmap_count;
}

__debug_optimize(3) uint8_t mm_get_section_count() {
    return mm_page_section_count;
}

__debug_optimize(3) const struct limine_framebuffer_response *boot_get_fb() {
    return &framebuffer_resp;
}

__debug_optimize(3) const struct limine_mp_response *boot_get_mp() {
    return mp_response;
}

__debug_optimize(3) const void *boot_get_rsdp() {
    return rsdp;
}

__debug_optimize(3) const void *boot_get_dtb() {
    return dtb;
}

__debug_optimize(3) uint64_t boot_get_time() {
    return boot_time;
}

__debug_optimize(3) uint64_t boot_get_slide() {
    return g_slide;
}

__debug_optimize(3) uint64_t mm_get_full_section_mask() {
    return mask_for_n_bits(mm_page_section_count);
}

__debug_optimize(3) static void sort_memmap_list() {
    // Sort the memmap list in ascending order based on range.front.
    for (uint64_t i = 0; i != mm_memmap_count - 1; i++) {
        for (uint64_t j = i + 1; j != mm_memmap_count; j++) {
            if (mm_memmap_list[i].range.front > mm_memmap_list[j].range.front) {
                swap(mm_memmap_list[i], mm_memmap_list[j]);
            }
        }
    }
}

__debug_optimize(3) static inline
bool is_usable_memmap(const struct mm_memmap *const memmap) {
    // TODO: Include bootloader-reclaimable memmaps in this check.
    return memmap->kind == MM_MEMMAP_KIND_USABLE;
}

void boot_init() {
    assert(hhdm_request.response != nullptr);
    assert(exec_addr_request.response != nullptr);
    assert(memmap_request.response != nullptr);
    assert(paging_mode_request.response != nullptr);

    HHDM_OFFSET = hhdm_request.response->offset;
    KERNEL_BASE = exec_addr_request.response->virtual_base;
    PAGING_MODE = paging_mode_request.response->mode;

#if defined(__x86_64__) || defined(__aarch64__) || defined(__riscv64) || \
    defined(__loongarch64)
    g_slide = KERNEL_BASE - 0xffffffff80000000;
#else
    #error "Unrecognized architecture when calculating slide"
#endif

    if (framebuffer_request.response != nullptr) {
        framebuffer_resp = *framebuffer_request.response;
    }

    mp_response = mp_request.response;
    if (dtb_request.response != nullptr &&
        dtb_request.response->dtb_ptr != nullptr)
    {
        dtb = dtb_request.response->dtb_ptr;
    }

    if (rsdp_request.response != nullptr &&
        rsdp_request.response->address != 0)
    {
        rsdp = phys_to_virt((uint64_t)rsdp_request.response->address);
    }

    const struct limine_memmap_response *const resp = memmap_request.response;

    uint8_t memmap_index = 0;
    ptrarr_foreach(resp->entries, resp->entry_count, entry) {
        if (memmap_index == countof(mm_memmap_list)) {
            panic("boot: too many memmaps\n");
        }

        const struct limine_memmap_entry *const memmap = *entry;
        struct range range = RANGE_EMPTY();

        if (!range_create_and_verify(memmap->base, memmap->length, &range)) {
            panic("boot: memmap overflows");
        }

        if (!range_align_out(range, /*boundary=*/PAGE_SIZE, &range)) {
            panic("boot: failed to align memmap");
        }

        if (range.front == 0) {
            if (range.size == PAGE_SIZE) {
                continue;
            }

            range = subrange_from_index(range, PAGE_SIZE);
        }

        mm_memmap_list[memmap_index].range = range;
        mm_memmap_list[memmap_index].kind = memmap->type + 1;

        memmap_index++;
    }

    mm_memmap_count = memmap_index;
    sort_memmap_list();

    // Merge usable, contiguous memmaps in the (now guaranteed to be sorted)
    // memmap list.

    ptrarr_foreach(&mm_memmap_list[1], mm_memmap_count, memmap) {
        struct mm_memmap *const prev_memmap = &memmap[-1];
        if (!is_usable_memmap(prev_memmap) || !is_usable_memmap(memmap)) {
            continue;
        }

        const struct range range = memmap->range;
        const struct range prev_range = prev_memmap->range;

        if (!range_overlaps(prev_range, range) &&
            !range_adjacent(prev_range, range))
         {
            continue;
        }

        prev_memmap->range = range_merge(prev_range, range);

        const struct mm_memmap *const end = &mm_memmap_list[mm_memmap_count];
        memmove(memmap, &memmap[1], distance(memmap, end));

        // We have removed the current memmap, so we need to decrement
        // the memmap count and index.

        mm_memmap_count--;
        memmap = prev_memmap;
    }

    // Usable (and bootloader-reclaimable) memmaps are sparsely located in the
    // physical memory-space, but are stored together (contiguously) in the
    // structpage-table in virtual-memory.
    // To accomplish this, assign each memmap placed in the structpage-table a
    // pfn that will be assigned to the first page residing in the memmap.

    uint8_t section_index = 0;
    uint64_t pfn = 0;

    ptrarr_foreach(mm_memmap_list, memmap_index, memmap) {
        // The page-section list is a list of usable memmaps.
        if (!is_usable_memmap(memmap)) {
            continue;
        }

        struct page_section *const section =
            &mm_page_section_list[section_index];

        page_section_init(section, /*zone=*/nullptr, memmap->range, pfn);
        for_upto_limit(MAX_ORDER, i) {
            list_init(&section->freelist_list[i].page_list);
            section->freelist_list[i].count = 0;
        }

        section_index++;
        pfn += PAGE_COUNT(memmap->range.size);
    }

    mm_page_section_count = section_index;
}

void boot_post_early_init() {
    printk(LOGLEVEL_INFO,
           "boot: there are %" PRIu8 " usable memmaps\n",
           mm_page_section_count);

#if !defined(__x86_64__)
    if (dtb_request.response == nullptr ||
        dtb_request.response->dtb_ptr == nullptr)
    {
        printk(LOGLEVEL_WARN, "boot: device tree is missing\n");
    }
#endif /* !defined(__x86_64__) */

    if (rsdp_request.response == nullptr || rsdp_request.response->address == 0)
    {
    #if !defined(__riscv64) && !defined(__aarch64__)
        panic("boot: acpi not found\n");
    #else
        printk(LOGLEVEL_WARN, "boot: acpi does not exist\n");
    #endif /* !defined(__riscv64) */
    }

    if (mp_response != nullptr) {
        printk(LOGLEVEL_INFO,
               "boot: found %" PRIu64 " cpus\n",
               mp_response->cpu_count);
    }

    if (boot_time_request.response == nullptr) {
        panic("boot: boot-time not found\n");
    }

    assert_msg(boot_time_request.response->timestamp > 0,
               "boot: boot-time is zero\n");

    boot_time = (uint64_t)boot_time_request.response->timestamp;
    printk(LOGLEVEL_INFO, "boot: boot timestamp is %" PRIu64 ": ", boot_time);

    const struct tm tm = tm_from_stamp((uint64_t)boot_time);
    printk_strftime(LOGLEVEL_INFO, "%c\n", &tm);
}

__debug_optimize(3)
void boot_remove_section(struct page_section *const section) {
    const uint64_t length =
        distance(section + 1, &mm_page_section_list[mm_page_section_count]);

    memmove(section, section + 1, length);
}

__debug_optimize(3)
struct page_section *boot_add_section_at(struct page_section *const section) {
    assert(mm_page_section_count < MM_MAX_SECTION_COUNT);

    const uint64_t length =
        distance(section, &mm_page_section_list[mm_page_section_count]);

    memmove(section + 1, section, length);
    mm_page_section_count++;

    return section;
}

__debug_optimize(3) void boot_recalculate_pfns() {
    struct page_section *section = mm_page_section_list;
    const struct page_section *const end =
        &mm_page_section_list[mm_page_section_count];

    for (uint64_t pfn = 0;
         section != end;
         section++, pfn += PAGE_COUNT(section->range.size))
    {
        section->pfn = pfn;
    }
}