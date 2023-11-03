/*
 * kernel/src/dev/virtio/device.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "device.h"

bool
virtio_device_shmem_region_map(struct virtio_device_shmem_region *const region)
{
    if (region->mapped) {
        return true;
    }

    struct mmio_region *const mmio =
        vmap_mmio(region->phys_range, PROT_READ | PROT_WRITE, /*flags=*/0);

    if (mmio == NULL) {
        printk(LOGLEVEL_WARN,
               "virtio-pci: failed to map shared-memory region\n");
        return false;
    }

    region->mmio = mmio;
    region->mapped = true;

    return true;
}

void
virtio_device_shmem_region_unmap(
    struct virtio_device_shmem_region *const region)
{
    if (!region->mapped) {
        return;
    }

    vunmap_mmio(region->mmio);

    region->mmio = NULL;
    region->mapped = false;

    return;
}