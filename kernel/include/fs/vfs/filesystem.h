/*
 * kernel/src/fs/vfs/filesystem.h
 * © suhas pai
 */

#pragma once
#include "node.h"

struct vfs_filesystem {
    void (*populate)(struct vfs_filesystem *, struct vfs_node *);

    struct vfs_node *
    (*create)(struct vfs_filesystem *fs,
              struct vfs_node *parent,
              struct string_view name,
              int mode);

    struct vfs_node *
    (*symlink)(struct vfs_filesystem *fs,
               struct vfs_node *node,
               struct string_view name,
               struct string_view target);

    struct vfs_node *
    (*link)(struct vfs_filesystem *fs,
            struct vfs_node *parent,
            struct string_view name,
            struct vfs_node *node);
};
