/*
 * kernel/dev/virtio/transport.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

struct virtio_device;
struct virtio_transport_ops {
    uint8_t (*read_device_status)(struct virtio_device *device);
    uint64_t (*read_device_features)(struct virtio_device *device);

    void (*write_device_status)(struct virtio_device *device, uint8_t status);
    void (*write_driver_features)(struct virtio_device *device, uint64_t feat);

    void
    (*read_device_info)(struct virtio_device *device,
                        uint16_t offset,
                        uint8_t size,
                        void *buf);

    void
    (*write_device_info)(struct virtio_device *device,
                         uint16_t offset,
                         uint8_t size,
                         const void *buf);

    void (*select_queue)(struct virtio_device *device, uint16_t queue);
    uint16_t (*selected_queue_max_size)(struct virtio_device *device);

    void
    (*set_selected_queue_size)(struct virtio_device *device, uint16_t size);

    void (*notify_queue)(struct virtio_device *device, uint16_t index);
    void (*enable_selected_queue)(struct virtio_device *device);

    void
    (*set_selected_queue_desc_phys)(struct virtio_device *device,
                                    uint64_t phys);

    void
    (*set_selected_queue_driver_phys)(struct virtio_device *device,
                                      uint64_t phys);

    void
    (*set_selected_queue_device_phys)(struct virtio_device *device,
                                      uint64_t phys);
};

#define VIRTIO_TRANSPORT_OPS_INIT() \
    ((struct virtio_transport_ops){ \
        .read_device_status = NULL, \
        .read_device_features = NULL, \
        .write_device_status = NULL, \
        .write_driver_features = NULL, \
        .read_device_info = NULL, \
        .write_device_info = NULL, \
        .select_queue = NULL, \
        .selected_queue_max_size = NULL, \
        .set_selected_queue_size = NULL, \
        .notify_queue = NULL, \
        .enable_selected_queue = NULL, \
        .set_selected_queue_desc_phys = NULL, \
        .set_selected_queue_driver_phys = NULL, \
        .set_selected_queue_device_phys = NULL \
    })

struct virtio_transport_ops virtio_transport_ops_for_mmio();
struct virtio_transport_ops virtio_transport_ops_for_pci();

#define virtio_device_read_status(device) \
    (device)->ops.read_device_status(device)
#define virtio_device_read_features(device) \
    (device)->ops.read_device_features(device)
#define virtio_device_write_status(device, status) \
    (device)->ops.write_device_status((device), (status))
#define virtio_device_write_features(device, features) \
    (device)->ops.write_driver_features((device), (features))
#define virtio_device_read_info(device, offset, size, buf) \
    (device)->ops.read_device_info((device), (offset), (size), (buf))
#define virtio_device_write_info(device, offset, size, buf) \
    (device)->ops.write_device_info((device), (offset), (size), (buf))

#define virtio_device_read_info_field(device, type, field) \
    ({ \
        uint64_t __result__; \
        virtio_device_read_info((device), \
                                offsetof(type, field), \
                                sizeof_field(type, field), \
                                &__result__); \
        (typeof_field(type, field))__result__; \
    })

#define virtio_device_write_info_field(device, type, field, value) \
    virtio_device_write_info((device), \
                             offsetof(type, field), \
                             sizeof(type, field), \
                             (value))

#define virtio_device_select_queue(device, index) \
    (device)->ops.select_queue((device), (index))
#define virtio_device_selected_queue_max_size(device) \
    (device)->ops.selected_queue_max_size((device))
#define virtio_device_set_selected_queue_size(device, size) \
    (device)->ops.set_selected_queue_size((device), (size))
#define virtio_device_notify_queue(device, index) \
    (device)->ops.notify_queue((device), (index))
#define virtio_device_enable_selected_queue(device) \
    (device)->ops.enable_selected_queue((device))
#define virtio_device_set_selected_queue_desc_phys(device, phys) \
    (device)->ops.set_selected_queue_desc_phys((device), (phys))
#define virtio_device_set_selected_queue_driver_phys(device, phys) \
    (device)->ops.set_selected_queue_driver_phys((device), (phys))
#define virtio_device_set_selected_queue_device_phys(device, phys) \
    (device)->ops.set_selected_queue_device_phys((device), (phys))
