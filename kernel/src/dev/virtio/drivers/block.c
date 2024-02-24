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
        // NUmber of logical blocks per physical block (log2)
        uint8_t physical_block_exp;
        // Offset of first aligned logical block
        uint8_t alignment_offset;
        // Suggested minimum I/O size in blocks
        le16_t min_io_size;
        // Optimal (suggested maximum) I/O size in blocks
        le32_t opt_io_size;
    } topology;

    uint8_t writeback;
    uint8_t unused0;

    le16_t num_queues;
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

#define virtio_block_read_config_field(device, field) \
    le_to_cpu( \
        virtio_device_read_info_field((device), \
                                      struct virtio_block_config, \
                                      field))

struct virtio_device *
virtio_block_driver_init(struct virtio_device *const device,
                         const uint64_t features)
{
    if (features & __VIRTIO_BLOCK_IS_READONLY) {
        printk(LOGLEVEL_INFO, "virtio-block: device is readonly\n");
    }

    const uint64_t capacity = virtio_block_read_config_field(device, capacity);
    printk(LOGLEVEL_INFO,
           "virtio-block: device has the following info:\n"
           "\tcapacity: " SIZE_UNIT_FMT "\n"
           "\tgeometry:\n"
           "\t\tcylinders: %" PRIu16 "\n"
           "\t\theads: %" PRIu8 "\n"
           "\t\tsectors: %" PRIu8 "\n"
           "\tblock-size: %" PRIu32 "\n"
           "\ttopology:\n"
           "\t\tphys-block count: %" PRIu8 "\n"
           "\t\tfirst-align-block offset: 0x%" PRIx8 "\n"
           "\t\tmin io-size: %" PRIu16 "\n"
           "\t\toptimal io-size: %" PRIu32 "\n"
           "\tqueue count: %" PRIu16 "\n",
           SIZE_UNIT_FMT_ARGS(capacity),
           virtio_block_read_config_field(device, geometry.cylinders),
           virtio_block_read_config_field(device, geometry.heads),
           virtio_block_read_config_field(device, geometry.sectors),
           virtio_block_read_config_field(device, block_size),
           virtio_block_read_config_field(device, topology.physical_block_exp),
           virtio_block_read_config_field(device, topology.alignment_offset),
           virtio_block_read_config_field(device, topology.min_io_size),
           virtio_block_read_config_field(device, topology.opt_io_size),
           virtio_block_read_config_field(device, num_queues));

    return NULL;
}