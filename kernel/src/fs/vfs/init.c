/*
 * kernel/src/fs/vfs/init.c
 * © suhas pai
 */

#include "fs/vfs/node.h"
#include "cpu/spinlock.h"

static struct spinlock g_lock = SPINLOCK_INIT();
static struct vfs_node *g_root = nullptr;

void vfs_init() {
    g_root = vfs_node_create(/*parent=*/nullptr, /*filesystem=*/nullptr, SV_EMPTY());
    assert_msg(g_root != nullptr, "vfs: failed to alloc root node. aborting init");

    (void)g_lock;
}