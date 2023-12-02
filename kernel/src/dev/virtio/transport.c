/*
 * kernel/src/dev/virtio/transport.c
 * © suhas pai
 */

#include "sys/mmio.h"
#include "device.h"

uint8_t virtio_pci_read_device_status(struct virtio_device *const device) {
    return mmio_read(&device->pci.common_cfg->device_status);
}

uint64_t virtio_pci_read_device_features(struct virtio_device *const device) {
    mmio_write(&device->pci.common_cfg->device_feature_select, 0);
    const uint32_t lower = mmio_read(&device->pci.common_cfg->device_feature);

    mmio_write(&device->pci.common_cfg->device_feature_select, 1);
    const uint32_t upper = mmio_read(&device->pci.common_cfg->device_feature);

    return ((uint64_t)upper << 32 | lower);
}

void
virtio_pci_write_driver_features(struct virtio_device *const device,
                                 const uint64_t value)
{
    mmio_write(&device->pci.common_cfg->driver_feature_select, 0);
    mmio_write(&device->pci.common_cfg->driver_feature, value);

    mmio_write(&device->pci.common_cfg->driver_feature_select, 1);
    mmio_write(&device->pci.common_cfg->driver_feature, value >> 32);
}

void
virtio_pci_write_device_status(struct virtio_device *const device,
                               const uint8_t status)
{
    mmio_write(&device->pci.common_cfg->device_status, status);
}

void
virtio_pci_read_device_info(struct virtio_device *const device,
                            const uint16_t offset,
                            const uint8_t size,
                            void *const buf)
{
    volatile void *const device_cfg =
        (volatile void *)device->pci.device_cfg.front;

    if (device_cfg != NULL) {
        switch (size) {
            case sizeof(uint8_t):
                *(uint8_t *)buf = mmio_read_8(device_cfg + offset);
                return;
            case sizeof(uint16_t):
                *(uint16_t *)buf = mmio_read_16(device_cfg + offset);
                return;
            case sizeof(uint32_t):
                *(uint32_t *)buf = mmio_read_32(device_cfg + offset);
                return;
            case sizeof(uint64_t):
                *(uint64_t *)buf = mmio_read_64(device_cfg + offset);
                return;
        }
    }

    verify_not_reached();
}

void
virtio_pci_write_device_info(struct virtio_device *const device,
                             const uint16_t offset,
                             const uint8_t size,
                             const void *const buf)
{
    volatile void *const device_cfg =
        (volatile void *)device->pci.device_cfg.front;

    if (device_cfg != NULL) {
        switch (size) {
            case sizeof(uint8_t):
                mmio_write_8(device_cfg + offset, *(const uint8_t *)buf);
                return;
            case sizeof(uint16_t):
                mmio_write_16(device_cfg + offset, *(const uint16_t *)buf);
                return;
            case sizeof(uint32_t):
                mmio_write_32(device_cfg + offset, *(const uint32_t *)buf);
                return;
            case sizeof(uint64_t):
                mmio_write_64(device_cfg + offset, *(const uint64_t *)buf);
                return;
        }
    }

    verify_not_reached();
}

void
virtio_pci_select_queue(struct virtio_device *const device,
                        const uint16_t queue)
{
    mmio_write(&device->pci.common_cfg->queue_select, le_to_cpu(queue));
}

uint16_t virtio_pci_selected_queue_max_size(struct virtio_device *const device) {
    return le_to_cpu(mmio_read(&device->pci.common_cfg->queue_size));
}

void
virtio_pci_set_selected_queue_size(struct virtio_device *const device,
                                   const uint16_t size)
{
    mmio_write(&device->pci.common_cfg->queue_size, cpu_to_le(size));
}

void
virtio_pci_notify_queue(struct virtio_device *const device,
                        const uint16_t index)
{

    mmio_write(&device->pci.notify_queue_select, cpu_to_le(index));
}

void virtio_pci_enable_selected_queue(struct virtio_device *const device) {
    mmio_write(&device->pci.common_cfg->queue_enable, /*value=*/cpu16_to_le(1));
}

void
virtio_pci_set_selected_queue_desc_phys(struct virtio_device *const device,
                                        const uint64_t phys)
{
    mmio_write(&device->pci.common_cfg->queue_desc, cpu_to_le(phys));
}

void
virtio_pci_set_selected_queue_driver_phys(struct virtio_device *const device,
                                          const uint64_t phys)
{
    mmio_write(&device->pci.common_cfg->queue_driver, cpu_to_le(phys));
}

void
virtio_pci_set_selected_queue_device_phys(struct virtio_device *const device,
                                          const uint64_t phys)
{
    mmio_write(&device->pci.common_cfg->queue_device, cpu_to_le(phys));
}

uint8_t virtio_mmio_read_device_status(struct virtio_device *const device) {
    (void)device;
    verify_not_reached();
}

uint64_t virtio_mmio_read_device_features(struct virtio_device *const device) {
    (void)device;
    verify_not_reached();
}

void
virtio_mmio_write_driver_features(struct virtio_device *const device,
                                  const uint64_t value)
{
    (void)device;
    (void)value;

    verify_not_reached();
}

void
virtio_mmio_write_device_status(struct virtio_device *const device,
                                const uint8_t status)
{
    (void)device;
    (void)status;

    verify_not_reached();
}

void
virtio_mmio_read_device_info(struct virtio_device *const device,
                             const uint16_t offset,
                             const uint8_t size,
                             void *const buf)
{
    (void)device;
    (void)offset;
    (void)size;
    (void)buf;

    verify_not_reached();
}

void
virtio_mmio_write_device_info(struct virtio_device *const device,
                              const uint16_t offset,
                              const uint8_t size,
                              const void *const buf)
{
    (void)device;
    (void)offset;
    (void)size;
    (void)buf;

    verify_not_reached();
}

void
virtio_mmio_select_queue(struct virtio_device *const device,
                         const uint16_t queue)
{
    (void)device;
    (void)queue;

    verify_not_reached();
}

uint16_t
virtio_mmio_selected_queue_max_size(struct virtio_device *const device) {
    (void)device;
    verify_not_reached();
}

void
virtio_mmio_set_selected_queue_size(struct virtio_device *const device,
                                    const uint16_t size)
{
    (void)device;
    (void)size;

    verify_not_reached();
}

void
virtio_mmio_notify_queue(struct virtio_device *const device,
                         const uint16_t index)
{
    (void)device;
    (void)index;

    verify_not_reached();
}

void virtio_mmio_enable_selected_queue(struct virtio_device *const device) {
    (void)device;
    verify_not_reached();
}

void
virtio_mmio_set_selected_queue_desc_phys(struct virtio_device *const device,
                                         const uint64_t phys)
{
    (void)device;
    (void)phys;

    verify_not_reached();
}

void
virtio_mmio_set_selected_queue_driver_phys(struct virtio_device *const device,
                                           const uint64_t phys)
{
    (void)device;
    (void)phys;

    verify_not_reached();
}

void
virtio_mmio_set_selected_queue_device_phys(struct virtio_device *const device,
                                           const uint64_t phys)
{
    (void)device;
    (void)phys;

    verify_not_reached();
}