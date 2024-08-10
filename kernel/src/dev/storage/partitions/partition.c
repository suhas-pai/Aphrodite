/*
 * kerne/src/dev/storage/partitions/partition.c
 * © suhas pai
 */

#include "partition.h"

__debug_optimize(3) void
partition_init(struct partition *const partition,
               const struct string name,
               struct storage_device *const device,
               const struct range range)
{
    list_init(&partition->list);

    partition->device = device;
    partition->name = name;
    partition->range = range;
}

__debug_optimize(3) uint64_t
partition_read(const struct partition *const partition,
               const struct range range,
               void *const buf)
{
    if (!range_has_index_range(partition->range, range)) {
        return 0;
    }

    struct storage_device *const device = partition->device;
    const struct range full_range = subrange_to_full(partition->range, range);

    return storage_device_read(device, full_range, buf);
}

__debug_optimize(3) uint64_t
partition_write(const struct partition *const partition,
                const struct range range,
                const void *const buf)
{
    if (!range_has_index_range(partition->range, range)) {
        return 0;
    }

    struct storage_device *const device = partition->device;
    const struct range full_range = subrange_to_full(partition->range, range);

    return storage_device_write(device, full_range, buf);
}