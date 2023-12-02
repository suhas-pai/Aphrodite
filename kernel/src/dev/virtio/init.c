/*
 * kernel/src/dev/virtio/init.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "mm/kmalloc.h"
#include "queue/split.h"

#include "driver.h"
#include "transport.h"

static struct list g_device_list = LIST_INIT(g_device_list);
static uint32_t g_device_count = 0;

bool
virtio_device_init_queues(struct virtio_device *const device,
                          const uint16_t queue_count)
{
    struct virtio_split_queue *const queue_list =
        kmalloc(sizeof(struct virtio_split_queue) * queue_count);

    if (queue_list == NULL) {
        printk(LOGLEVEL_WARN,
               "virtio-pci: failed to allocate space for %" PRIu16 " "
               "virtio_queue objects\n",
               queue_count);
        return false;
    }

    for (uint16_t index = 0; index != queue_count; index++) {
        if (!virtio_split_queue_init(device, &queue_list[index], index)) {
            kfree(queue_list);
            return false;
        }
    }

    device->queue_list = queue_list;
    device->queue_count = queue_count;

    return true;
}

struct virtio_device *virtio_device_init(struct virtio_device *const device) {
    struct virtio_device *iter = NULL;
    list_foreach(iter, &g_device_list, list) {
        if (iter->kind == device->kind) {
            return NULL;
        }
    }

    const struct virtio_driver_info *const driver =
        &virtio_drivers[device->kind];

    if (driver->init == NULL) {
        printk(LOGLEVEL_WARN, "virtio-pci: ignoring device, no driver found\n");
        return NULL;
    }

    // 1. Reset the device.
    virtio_device_write_status(device, /*status=*/0);

    // 2. Set the ACKNOWLEDGE status bit: the guest OS has noticed the device.
    uint8_t status = virtio_device_read_status(device);
    virtio_device_write_status(device, status | __VIRTIO_DEVSTATUS_ACKNOWLEDGE);

    // 3. Set the DRIVER status bit: the guest OS knows how to drive the device.
    status = virtio_device_read_status(device);
    virtio_device_write_status(device, status | __VIRTIO_DEVSTATUS_DRIVER);

    // 4. Read device feature bits, and write the subset of feature bits
    // understood by the OS and driver to the device. During this step the
    // driver MAY read (but MUST NOT write) the device-specific configuration
    // fields to check that it can support the device before accepting it.

    uint64_t features = virtio_device_read_features(device);
    if ((features & driver->required_features) != driver->required_features) {
        printk(LOGLEVEL_WARN,
               "virtio-pci: device is missing required features, features: "
               "%" PRIu64 "\n",
               features);
        return NULL;
    }

    virtio_device_write_features(device, features);

    // The transitional driver MUST execute the initialization sequence as
    // described in 3.1 but omitting the steps 5 and 6.

    const bool is_legacy = (features & __VIRTIO_DEVFEATURE_VERSION_1) == 0;
    if (!is_legacy) {
        // 5. Set the FEATURES_OK status bit. The driver MUST NOT accept new
        // feature bits after this step.

        status = virtio_device_read_status(device);
        status |= __VIRTIO_DEVSTATUS_FEATURES_OK;

        virtio_device_write_status(device, status);

        // 6. Re-read device status to ensure the FEATURES_OK bit is still set:
        // otherwise, the device does not support our subset of features and the
        // device is unusable.

        status = virtio_device_read_status(device);
        if ((status & __VIRTIO_DEVSTATUS_FEATURES_OK) == 0) {
            printk(LOGLEVEL_WARN, "virtio-pci: failed to accept features\n");
            return NULL;
        }
    } else {
        printk(LOGLEVEL_INFO, "virtio-pci: device is legacy\n");
    }

    status = virtio_device_read_status(device);
    virtio_device_write_status(device, status | __VIRTIO_DEVSTATUS_DRIVER_OK);

    if (driver->virtqueue_count != 0) {
        if (!virtio_device_init_queues(device, driver->virtqueue_count)) {
            status |= __VIRTIO_DEVSTATUS_FAILED;
            virtio_device_write_status(device, status);

            return NULL;
        }
    }

    struct virtio_device *const ret_device = driver->init(device, features);
    if (ret_device == NULL) {
        virtio_device_write_status(device, status | __VIRTIO_DEVSTATUS_FAILED);
        return NULL;
    }

    list_add(&g_device_list, &ret_device->list);
    g_device_count++;

    return ret_device;
}