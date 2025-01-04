/*
 * kernel/src/fs/vfs/node.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/hashmap.h"
#include "lib/adt/string.h"

struct vfs_filesystem;
struct vfs_node {
    struct vfs_node *parent;
    struct vfs_filesystem *filesystem;

    struct hashmap children;
    struct string name;
};

struct vfs_node *
vfs_node_create(struct vfs_node *parent,
                struct vfs_filesystem *filesystem,
                struct string_view name);
