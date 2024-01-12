/*
 * kernel/src/dev/virtio/drivers/scsi.c
 * © suhas pai
 */

#include "../init.h"
#include "../transport.h"

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

// Command-specific response values
enum virtio_scsi_status {
    // When the request was completed and the status byte is filled with a SCSI
    // status code (not necessarily “GOOD”).
    __VIRTIO_SCSI_S_OK = 1 << 0,

    /*
     * If the content of the CDB (such as the allocation length, parameter
     * length or transfer size) requires more data than is available in the
     * datain and dataout buffers.
     */
    __VIRTIO_SCSI_S_OVERRUN = 1 << 1,

    // If the request was cancelled due to an ABORT TASK or ABORT TASK SET task
    // management function.
    __VIRTIO_SCSI_S_ABORTED = 1 << 2,

    // If the request was never processed because the target indicated by lun
    // does not exist.
    __VIRTIO_SCSI_S_BAD_TARGET = 1 << 3,

    // If the request was cancelled due to a bus or device reset (including a
    // task management function).
    __VIRTIO_SCSI_S_RESET = 1 << 4,

    // If the request failed but retrying on the same path is likely to work
    __VIRTIO_SCSI_S_BUSY = 1 << 5,

    // If the request failed due to a problem in the connection between the host
    // and the target (severed link)
    __VIRTIO_SCSI_S_TRANSPORT_FAILURE = 1 << 6,

    // If the target is suffering a failure and to tell the driver not to retry
    // on other paths.
    __VIRTIO_SCSI_S_TARGET_FAILURE = 1 << 7,

    // If the nexus is suffering a failure but retrying on other paths might
    // yield a different result
    __VIRTIO_SCSI_S_NEXUS_FAILURE = 1 << 8,

    /*
     * For other host or driver error. In particular, if neither dataout nor
     * datain is empty, and the VIRTIO_SCSI_F_INOUT feature has not been
     * negotiated, the request will be immediately returned with a response
     * equal to VIRTIO_SCSI_S_FAILURE.
     */
    __VIRTIO_SCSI_S_FAILURE = 1 << 9,
};

enum virtio_scsi_task_attributes {
    __VIRTIO_SCSI_TASK_ATTR_SIMPLE = 1 << 0,
    __VIRTIO_SCSI_TASK_ATTR_ORDERED = 1 << 1,
    __VIRTIO_SCSI_TASK_ATTR_HEAD = 1 << 2,
    __VIRTIO_SCSI_TASK_ATTR_ACA = 1 << 3,
};

#define virtio_scsi_read_config_field(device, field) \
    le_to_cpu( \
        virtio_device_read_info_field((device), \
                                      struct virtio_scsi_config, \
                                      field))

#define VIRTIO_SCSI_QUEUE_MAX_COUNT (uint16_t)32

struct virtio_device *
virtio_scsi_driver_init(struct virtio_device *const device,
                        const uint64_t features)
{
    (void)features;

    uint32_t queue_count = virtio_scsi_read_config_field(device, num_queues);
    if (queue_count > VIRTIO_SCSI_QUEUE_MAX_COUNT) {
        printk(LOGLEVEL_WARN,
               "virtio-scsi: too many queues requested (%" PRIu32 "), falling "
               "back to driver max: %" PRIu16 "\n",
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