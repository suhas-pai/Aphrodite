/*
 * kernel/dev/virtio/queue/split.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "mm/page_alloc.h"

#include "split.h"

bool
virtio_split_queue_init(struct virtio_device *const device,
                        struct virtio_split_queue *const queue,
                        const uint16_t queue_index)
{
    virtio_device_select_queue(device, queue_index);
    const uint16_t desc_count =
        min(virtio_device_selected_queue_max_size(device),
            VIRTQ_MAX_DESC_COUNT);

    _Static_assert(
        sizeof(struct virtq_desc) * VIRTQ_MAX_DESC_COUNT <= (PAGE_SIZE << 1),
        "virto-queue: desc_pages allocation order needs to be increased");

    _Static_assert(
        sizeof(uint16_t) * (3 + VIRTQ_MAX_DESC_COUNT) <= PAGE_SIZE,
        "virto-queue: avail_pages allocation order needs to be increased");

    _Static_assert(
        ((sizeof(uint16_t) * 3) +
            (sizeof(struct virtq_used_elem) * VIRTQ_MAX_DESC_COUNT))
                <= (PAGE_SIZE << 1),
        "virto-queue: used_pages allocation order needs to be increased");

    // The max possible size of the descriptor list is actually 8192 bytes.
    struct page *desc_pages = NULL;
    uint8_t desc_pages_order = 0;

    if (sizeof(struct virtq_desc) * desc_count > PAGE_SIZE) {
        desc_pages = alloc_pages(PAGE_STATE_USED, __ALLOC_ZERO, /*order=*/1);
        desc_pages_order = 1;
    } else {
        desc_pages = alloc_page(PAGE_STATE_USED, __ALLOC_ZERO);
    }

    if (desc_pages == NULL) {
        printk(LOGLEVEL_WARN,
               "virtio-queue: failed to allocate desc page for queue at index "
               "%" PRIu16 "\n",
               queue_index);
        return false;
    }

    // The max possible size of the descriptor list is actually 1030 bytes.
    struct page *const avail_page = alloc_page(PAGE_STATE_USED, __ALLOC_ZERO);
    if (avail_page == NULL) {
        free_pages(desc_pages, desc_pages_order);
        printk(LOGLEVEL_WARN,
               "virtio-queue: failed to allocate avail page for queue at index "
               "%" PRIu16 "\n",
               queue_index);
        return false;
    }

    // The max possible used_ring_size should be 4102 bytes.
    const uint16_t used_ring_size =
        (sizeof(uint16_t) * 3) + (sizeof(struct virtq_used_elem) * desc_count);

    uint8_t used_pages_order = 0;
    if (used_ring_size > PAGE_SIZE) {
        used_pages_order++;
    }

    struct page *const used_pages =
        alloc_pages(PAGE_STATE_USED, __ALLOC_ZERO, used_pages_order);

    if (used_pages == NULL) {
        free_page(avail_page);
        free_pages(desc_pages, desc_pages_order);

        printk(LOGLEVEL_WARN,
               "virtio-queue: failed to allocate used page for queue at index "
               "%" PRIu16 "\n",
               queue_index);

        return false;
    }

    virtio_device_set_selected_queue_desc_phys(device, page_to_phys(desc_pages));
    virtio_device_set_selected_queue_driver_phys(device,
                                                 page_to_phys(avail_page));
    virtio_device_set_selected_queue_device_phys(device,
                                                 page_to_phys(used_pages));

    virtio_device_enable_selected_queue(device);

    // Set the next-indices of each virtio-desc to point to the desc right after
    // Outside the loop, set the next index for the last desc to 0.

    struct virtq_desc *const begin = page_to_virt(desc_pages);
    const struct virtq_desc *const end = begin + (desc_count - 1);

    struct virtq_desc *iter = begin;
    uint16_t next_index = 1;

    for (; iter != end; iter++, next_index++) {
        iter->next = next_index;
    }

    iter->next = 0;

    queue->desc_pages = desc_pages;
    queue->avail_page = avail_page;
    queue->used_pages = used_pages;

    queue->desc_pages_order = desc_pages_order;
    queue->used_pages_order = used_pages_order;

    return true;
}