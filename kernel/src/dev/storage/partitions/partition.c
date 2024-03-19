/*
 * kerne/src/dev/storage/partitions/partition.c
 * © suhas pai
 */

#include "partition.h"

__optimize(3) bool
partition_init(struct partition *const partition,
                 struct storage_device *const device,
                 const struct range lba_range)
{
    list_init(&partition->list);

    partition->device = device;
    partition->lba_range = lba_range;

    return true;
}

__optimize(3) bool
partition_read(const struct partition *const partition,
               void *const buf,
               const struct range range)
{
    struct storage_device *const device = partition->device;
    const struct range lba_range = range_divide_out(range, device->lba_size);

    if (!range_has_index_range(partition->lba_range, lba_range)) {
        return false;
    }

    const struct range full_range =
        RANGE_INIT(partition->lba_range.front * device->lba_size + range.front,
                   range.size);

    return device->read(device, buf, full_range);
}

__optimize(3) bool
partition_write(const struct partition *const partition,
                const void *const buf,
                const struct range range)
{
    struct storage_device *const device = partition->device;
    const struct range lba_range = range_divide_out(range, device->lba_size);

    if (!range_has_index_range(partition->lba_range, lba_range)) {
        return false;
    }

    const struct range full_range =
        RANGE_INIT(partition->lba_range.front * device->lba_size + range.front,
                   range.size);

    return device->write(device, buf, full_range);
}