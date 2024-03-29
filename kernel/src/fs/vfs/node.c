/*
 * kernel/src/fs/vfs/node.c
 * © suhas pai
 */

#include "mm/kmalloc.h"
#include "node.h"

struct vfs_node *
vfs_node_create(struct vfs_node *const parent,
                struct vfs_filesystem *const filesystem,
                const struct string_view name)
{
    struct vfs_node *const result = kmalloc(sizeof(*result));
    if (result == NULL) {
        return NULL;
    }

    result->name = string_alloc(name);
    if (name.length != 0 && string_length(result->name) == 0) {
        kfree(result);
        return NULL;
    }

    result->parent = parent;
    result->filesystem = filesystem;

    return result;
}