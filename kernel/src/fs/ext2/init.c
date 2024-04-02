/*
 * kernel/src/fs/ext2/init.c
 * © suhas pai
 */

#include "dev/storage/partitions/partition.h"

#include "dev/printk.h"
#include "fs/driver.h"

#include "structs.h"

__optimize(3) static bool try_init(struct partition *const partition) {
    struct ext2fs_superblock superblock;
    const struct range range = RANGE_INIT(0, sizeof(superblock));

    if (partition_read(partition, &superblock, range) != range.size) {
        printk(LOGLEVEL_WARN, "ext2fs: failed to read superblock\n");
        return false;
    }

    return superblock.signature == EXT2FS_SUPERBLOCK_SIGNATURE;
}

__fs_driver static const struct fs_driver driver = {
    .name = SV_STATIC("ext2"),
    .try_init = try_init
};