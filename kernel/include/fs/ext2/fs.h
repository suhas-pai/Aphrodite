/*
 * kernel/src/fs/ext2/fs.h
 * © suhas pai
 */

#pragma once
#include "fs/vfs/filesystem.h"

struct ext2_fs {
    struct vfs_filesystem vfs;
};
