/*
 * kernel/src/fs/fat32/fs.c
 * © suhas pai
 */

#include "dev/storage/partitions/partition.h"
#include "dev/printk.h"

#include "fs/driver.h"
#include "structs.h"

__debug_optimize(3) static bool try_init(struct partition *const partition) {
    struct fat32_bootrecord record;
    const struct range range = RANGE_INIT(0, sizeof(record));

    if (partition_read(partition, range, &record) != range.size) {
        printk(LOGLEVEL_WARN, "fat32: failed to read boot record\n");
        return false;
    }

    return record.signature == FAT32_BOOTRECORD_SIGNATURE
        && sv_equals(sv_of_carr(record.identifier),
                     SV_STATIC(FAT32_BOOTRECORD_IDENTIFIER));
}

__fs_driver static const struct fs_driver driver = {
    .name = SV_STATIC("fat32"),
    .try_init = try_init
};