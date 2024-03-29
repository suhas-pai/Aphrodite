/*
 * kernel/src/fs/fat32/fs.h
 * © suhas pai
 */

#pragma once
#include "fs/vfs/filesystem.h"

struct fat32_fs {
    struct vfs_filesystem vfs;
};
