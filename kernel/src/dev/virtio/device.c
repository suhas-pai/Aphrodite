/*
 * kernel/src/dev/virtio/device.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "device.h"

bool
virtio_device_shmem_region_map(struct virtio_device_shmem_region *const region)
{
    if (region->mmio != NULL) {
        return true;
    }

    struct mmio_region *const mmio =
        vmap_mmio(region->phys_range, PROT_READ | PROT_WRITE, /*flags=*/0);

    if (mmio == NULL) {
        printk(LOGLEVEL_WARN,
               "virtio-device: failed to map shared-memory region\n");
        return false;
    }

    region->mmio = mmio;
    return true;
}

void
virtio_device_shmem_region_unmap(
    struct virtio_device_shmem_region *const region)
{
    if (region->mmio == NULL) {
        return;
    }

    vunmap_mmio(region->mmio);
    region->mmio = NULL;

    return;
}

__optimize(3) void virtio_device_destroy(struct virtio_device *const device) {
    array_foreach(&device->shmem_regions,
                  struct virtio_device_shmem_region,
                  shmem)
    {
        virtio_device_shmem_region_unmap(shmem);
    }

    array_destroy(&device->shmem_regions);
    array_destroy(&device->vendor_cfg_list);

    if (device->transport_kind == VIRTIO_DEVICE_TRANSPORT_MMIO) {
        vunmap_mmio(device->mmio.region);
    }
}