/*
 * kernel/src/fs/vfs/init.c
 * © suhas pai
 */

#include "cpu/spinlock.h"
#include "node.h"

static struct spinlock g_lock = SPINLOCK_INIT();
static struct vfs_node *g_root = NULL;

void vfs_init() {
    g_root = vfs_node_create(/*parent=*/NULL, /*filesystem=*/NULL, SV_EMPTY());
    assert_msg(g_root != NULL, "vfs: failed to alloc root node. aborting init");

    (void)g_lock;
}