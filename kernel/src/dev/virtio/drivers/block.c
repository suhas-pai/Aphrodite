/*
 * kernel/src/dev/virtio/drivers/block.c
 * © suhas pai
 */

#include "../transport.h"

#include "dev/printk.h"
#include "lib/size.h"

#include "block.h"

struct virtio_block_config {
    le64_t capacity;
    le32_t size_max;
    le32_t seg_max;

    struct virtio_block_geometry {
        le16_t cylinders;

        uint8_t heads;
        uint8_t sectors;
    } geometry;

    le32_t block_size;
    struct virtio_block_topology {
        // # of logical blocks per physical block (log2)
        uint8_t physical_block_exp;
        // offset of first aligned logical block
        uint8_t alignment_offset;
        // suggested minimum I/O size in blocks
        le16_t min_io_size;
        // optimal (suggested maximum) I/O size in blocks
        le32_t opt_io_size;
    } topology;

    uint8_t writeback;
    uint8_t unused0;
    uint16_t num_queues;

    le32_t max_discard_sectors;
    le32_t max_discard_seg;
    le32_t discard_sector_alignment;
    le32_t max_write_zeroes_sectors;
    le32_t max_write_zeroes_seg;

    uint8_t write_zeroes_may_unmap;
    uint8_t unused1[3];

    le32_t max_secure_erase_sectors;
    le32_t max_secure_erase_seg;
    le32_t secure_erase_sector_alignment;
};

struct virtio_device *
virtio_block_driver_init(struct virtio_device *const device,
                         const uint64_t features)
{
    if (features & __VIRTIO_BLOCK_IS_READONLY) {
        printk(LOGLEVEL_INFO, "virtio-block: device is readonly\n");
    }

    const le64_t capacity =
        le_to_cpu(
            virtio_device_read_info_field(device,
                                          struct virtio_block_config,
                                          capacity));

    printk(LOGLEVEL_INFO,
           "virtio-block: device has the following info:\n"
           "\tcapacity: " SIZE_TO_UNIT_FMT "\n",
           SIZE_TO_UNIT_FMT_ARGS(capacity));

    return NULL;
}