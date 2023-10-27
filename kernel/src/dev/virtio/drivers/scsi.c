/*
 * kernel/dev/virtio/drivers/scsi.c
 * © suhas pai
 */

#include "../init.h"

#include "dev/printk.h"
#include "scsi.h"

struct virtio_scsi_config {
    const le32_t num_queues;
    const le32_t seg_max;
    const le32_t max_sectors;
    const le32_t cmd_per_lun;
    const le32_t event_info_size;

    le32_t sense_size;
    le32_t cdb_size;

    const le16_t max_channel;
    const le16_t max_target;
    const le32_t max_lun;
};

#define VIRTIO_SCSI_QUEUE_MAX_COUNT (uint16_t)32

struct virtio_device *
virtio_scsi_driver_init(struct virtio_device *const device,
                        const uint64_t features)
{
    (void)features;
    uint32_t queue_count =
        le_to_cpu(
            virtio_device_read_info_field(device,
                                          struct virtio_scsi_config,
                                          num_queues));

    if (queue_count > VIRTIO_SCSI_QUEUE_MAX_COUNT) {
        printk(LOGLEVEL_WARN,
               "virtio-scsi: too many queues requested (%" PRIu32 "), falling "
               "instead to driver max: %" PRIu16 "\n",
               queue_count,
               VIRTIO_SCSI_QUEUE_MAX_COUNT);

        queue_count = VIRTIO_SCSI_QUEUE_MAX_COUNT;
    }

    printk(LOGLEVEL_INFO,
           "virtio-scsi: initializing %" PRIu32 " queues\n",
           queue_count);

    if (!virtio_device_init_queues(device, queue_count)) {
        return NULL;
    }

    return NULL;
}