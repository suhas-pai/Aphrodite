/*
 * kernel/src/dev/virtio/transport.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

struct virtio_device;

uint8_t virtio_pci_read_device_status(struct virtio_device *device);
uint64_t virtio_pci_read_device_features(struct virtio_device *device);

void
virtio_pci_write_driver_features(struct virtio_device *device, uint64_t value);

void
virtio_pci_write_device_status(struct virtio_device *device, uint8_t status);

void
virtio_pci_read_device_info(struct virtio_device *device,
                            uint16_t offset,
                            uint8_t size,
                            void *buf);

void
virtio_pci_write_device_info(struct virtio_device *device,
                             uint16_t offset,
                             uint8_t size,
                             const void *buf);

void virtio_pci_select_queue(struct virtio_device *device, uint16_t queue);
uint16_t virtio_pci_selected_queue_max_size(struct virtio_device *device);

void
virtio_pci_set_selected_queue_size(struct virtio_device *device, uint16_t size);

void virtio_pci_notify_queue(struct virtio_device *device, uint16_t index);
void virtio_pci_enable_selected_queue(struct virtio_device *device);

void
virtio_pci_set_selected_queue_desc_phys(struct virtio_device *device,
                                        uint64_t phys);

void
virtio_pci_set_selected_queue_driver_phys(struct virtio_device *device,
                                          uint64_t phys);

void
virtio_pci_set_selected_queue_device_phys(struct virtio_device *device,
                                          uint64_t phys);

uint8_t virtio_mmio_read_device_status(struct virtio_device *device);
uint64_t virtio_mmio_read_device_features(struct virtio_device *device);

void
virtio_mmio_write_driver_features(struct virtio_device *device, uint64_t value);

void
virtio_mmio_write_device_status(struct virtio_device *device, uint8_t status);

void
virtio_mmio_read_device_info(struct virtio_device *device,
                             uint16_t offset,
                             uint8_t size,
                             void *buf);

void
virtio_mmio_write_device_info(struct virtio_device *device,
                              uint16_t offset,
                              uint8_t size,
                              const void *buf);

void virtio_mmio_select_queue(struct virtio_device *device, uint16_t queue);
uint16_t virtio_mmio_selected_queue_max_size(struct virtio_device *device);

void
virtio_mmio_set_selected_queue_size(struct virtio_device *device,
                                    uint16_t size);

void virtio_mmio_notify_queue(struct virtio_device *device, uint16_t index);
void virtio_mmio_enable_selected_queue(struct virtio_device *device);

void
virtio_mmio_set_selected_queue_desc_phys(struct virtio_device *device,
                                         uint64_t phys);

void
virtio_mmio_set_selected_queue_driver_phys(struct virtio_device *device,
                                           uint64_t phys);

void
virtio_mmio_set_selected_queue_device_phys(struct virtio_device *device,
                                           uint64_t phys);

#define virtio_device_read_status(device) \
    ((device)->transport_kind == VIRTIO_DEVICE_TRANSPORT_PCI ? \
        virtio_pci_read_device_status(device) : \
        virtio_mmio_read_device_status(device))

#define virtio_device_read_features(device) \
    ((device)->transport_kind == VIRTIO_DEVICE_TRANSPORT_PCI ? \
        virtio_pci_read_device_features(device) : \
        virtio_mmio_read_device_features(device))

#define virtio_device_write_status(device, status) \
    ((device)->transport_kind == VIRTIO_DEVICE_TRANSPORT_PCI ? \
        virtio_pci_write_device_status((device), (status)) : \
        virtio_mmio_write_device_status((device), (status)))

#define virtio_device_write_features(device, features) \
    ((device)->transport_kind == VIRTIO_DEVICE_TRANSPORT_PCI ? \
        virtio_pci_write_driver_features((device), (features)) : \
        virtio_mmio_write_driver_features((device), (features)))

#define virtio_device_read_info(device, offset, size, buf) \
    ((device)->transport_kind == VIRTIO_DEVICE_TRANSPORT_PCI ? \
        virtio_pci_read_device_info((device), (offset), (size), (buf)) : \
        virtio_mmio_read_device_info((device), (offset), (size), (buf)))

#define virtio_device_write_info(device, offset, size, buf) \
    ((device)->transport_kind == VIRTIO_DEVICE_TRANSPORT_PCI ? \
        virtio_pci_write_device_info((device), (offset), (size), (buf)) : \
        virtio_mmio_write_device_info((device), (offset), (size), (buf)))

#define virtio_device_read_info_field(device, type, field) ({ \
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
    ((device)->transport_kind == VIRTIO_DEVICE_TRANSPORT_PCI ? \
        virtio_pci_select_queue((device), (index)) : \
        virtio_mmio_select_queue((device), (index)))
#define virtio_device_selected_queue_max_size(device) \
    ((device)->transport_kind == VIRTIO_DEVICE_TRANSPORT_PCI ? \
        virtio_pci_selected_queue_max_size((device)) : \
        virtio_mmio_selected_queue_max_size((device)))
#define virtio_device_set_selected_queue_size(device, size) \
    ((device)->transport_kind == VIRTIO_DEVICE_TRANSPORT_PCI ? \
        virtio_pci_set_selected_queue_size((device), (size)) : \
        virtio_mmio_set_selected_queue_size((device), (size)))
#define virtio_device_notify_queue(device, index) \
    ((device)->transport_kind == VIRTIO_DEVICE_TRANSPORT_PCI ? \
        virtio_pci_notify_queue((device), (index)) : \
        virtio_mmio_notify_queue((device), (index)))
#define virtio_device_enable_selected_queue(device) \
    ((device)->transport_kind == VIRTIO_DEVICE_TRANSPORT_PCI ? \
        virtio_pci_enable_selected_queue((device)) : \
        virtio_mmio_enable_selected_queue((device)))
#define virtio_device_set_selected_queue_desc_phys(device, phys) \
    ((device)->transport_kind == VIRTIO_DEVICE_TRANSPORT_PCI ? \
        virtio_pci_set_selected_queue_desc_phys((device), (phys)) : \
        virtio_mmio_set_selected_queue_desc_phys((device), (phys)))
#define virtio_device_set_selected_queue_driver_phys(device, phys) \
    ((device)->transport_kind == VIRTIO_DEVICE_TRANSPORT_PCI ? \
        virtio_pci_set_selected_queue_driver_phys((device), (phys)) : \
        virtio_mmio_set_selected_queue_driver_phys((device), (phys)))
#define virtio_device_set_selected_queue_device_phys(device, phys) \
    ((device)->transport_kind == VIRTIO_DEVICE_TRANSPORT_PCI ? \
        virtio_pci_set_selected_queue_device_phys((device), (phys)) : \
        virtio_mmio_set_selected_queue_device_phys((device), (phys)))
