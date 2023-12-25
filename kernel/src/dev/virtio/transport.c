/*
 * kernel/src/dev/virtio/transport.c
 * © suhas pai
 */

#include "dev/printk.h"
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

uint16_t
virtio_pci_selected_queue_max_size(struct virtio_device *const device) {
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
    const uint32_t offset =
        device->pci.common_cfg->queue_notify_off *
        device->pci.notify_off_multiplier;
    volatile uint16_t *const ptr =
        (void *)device->pci.notify_cfg_range.front + offset;

    mmio_write(ptr, cpu_to_le(index));
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
    return mmio_read(&device->mmio.header->status);
}

uint64_t virtio_mmio_read_device_features(struct virtio_device *const device) {
    return mmio_read(&device->mmio.header->device_features);
}

void
virtio_mmio_write_driver_features(struct virtio_device *const device,
                                  const uint64_t value)
{
    mmio_write(&device->mmio.header->driver_features_select, 0);
    mmio_write(&device->mmio.header->driver_features, (uint32_t)value);

    mmio_write(&device->mmio.header->driver_features_select, 1);
    mmio_write(&device->mmio.header->driver_features, value >> 32);
}

void
virtio_mmio_write_device_status(struct virtio_device *const device,
                                const uint8_t status)
{
    mmio_write(&device->mmio.header->status, status);
}

void
virtio_mmio_read_device_info(struct virtio_device *const device,
                             const uint16_t offset,
                             const uint8_t size,
                             void *const buf)
{
    const struct range range = RANGE_INIT(offset, size);
    const struct range config_range =
        subrange_from_index(mmio_region_get_range(device->mmio.region),
                            offsetof(struct virtio_mmio_device, config_space));

    if (!range_has_index_range(config_range, range)) {
        printk(LOGLEVEL_WARN,
               "virtio-mmio: attempting to read bytes in range " RANGE_FMT " "
               "falls outside of the config-space range " RANGE_FMT "\n",
               RANGE_FMT_ARGS(range),
               RANGE_FMT_ARGS(config_range));
        return;
    }

    volatile void *const ptr = device->mmio.header->config_space;
    switch (size) {
        case sizeof(uint8_t):
            *(uint8_t *)buf = mmio_read_8(ptr + offset);
            return;
        case sizeof(uint16_t):
            *(uint16_t *)buf = mmio_read_16(ptr + offset);
            return;
        case sizeof(uint32_t):
            *(uint32_t *)buf = mmio_read_32(ptr + offset);
            return;
        case sizeof(uint64_t):
            *(uint64_t *)buf = mmio_read_64(ptr + offset);
            return;
    }

    verify_not_reached();
}

void
virtio_mmio_write_device_info(struct virtio_device *const device,
                              const uint16_t offset,
                              const uint8_t size,
                              const void *const buf)
{
    const struct range range = RANGE_INIT(offset, size);
    const struct range config_range =
        subrange_from_index(mmio_region_get_range(device->mmio.region),
                            offsetof(struct virtio_mmio_device, config_space));

    if (!range_has_index_range(config_range, range)) {
        printk(LOGLEVEL_WARN,
               "virtio-mmio: attempting to read bytes in range " RANGE_FMT " "
               "falls outside of the config-space range " RANGE_FMT "\n",
               RANGE_FMT_ARGS(range),
               RANGE_FMT_ARGS(config_range));
        return;
    }

    volatile void *const ptr = device->mmio.header->config_space;
    switch (size) {
        case sizeof(uint8_t):
            mmio_write_8(ptr + offset, *(const uint8_t *)buf);
            return;
        case sizeof(uint16_t):
            mmio_write_16(ptr + offset, *(const uint16_t *)buf);
            return;
        case sizeof(uint32_t):
            mmio_write_32(ptr + offset, *(const uint32_t *)buf);
            return;
        case sizeof(uint64_t):
            mmio_write_64(ptr + offset, *(const uint64_t *)buf);
            return;
    }

    verify_not_reached();
}

void
virtio_mmio_select_queue(struct virtio_device *const device,
                         const uint16_t queue)
{
    mmio_write(&device->mmio.header->queue_select, queue);
}

uint16_t
virtio_mmio_selected_queue_max_size(struct virtio_device *const device) {
    return mmio_read(&device->mmio.header->queue_num_max);
}

void
virtio_mmio_set_selected_queue_size(struct virtio_device *const device,
                                    const uint16_t size)
{
    mmio_write(&device->mmio.header->queue_num, size);
}

void
virtio_mmio_notify_queue(struct virtio_device *const device,
                         const uint16_t index)
{
    mmio_write(&device->mmio.header->queue_notify, index);
}

void virtio_mmio_enable_selected_queue(struct virtio_device *const device) {
    mmio_write(&device->mmio.header->queue_ready, 1);
}

void
virtio_mmio_set_selected_queue_desc_phys(struct virtio_device *const device,
                                         const uint64_t phys)
{
    mmio_write(&device->mmio.header->queue_desc_low, (uint32_t)phys);
    mmio_write(&device->mmio.header->queue_desc_high, phys >> 32);
}

void
virtio_mmio_set_selected_queue_driver_phys(struct virtio_device *const device,
                                           const uint64_t phys)
{
    mmio_write(&device->mmio.header->queue_driver_low, (uint32_t)phys);
    mmio_write(&device->mmio.header->queue_driver_high, phys >> 32);
}

void
virtio_mmio_set_selected_queue_device_phys(struct virtio_device *const device,
                                           const uint64_t phys)
{
    mmio_write(&device->mmio.header->queue_device_low, (uint32_t)phys);
    mmio_write(&device->mmio.header->queue_device_high, phys >> 32);
}