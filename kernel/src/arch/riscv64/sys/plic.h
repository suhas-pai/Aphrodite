/*
 * kernel/src/arch/riscv64/sys/plic.h
 * © suhas pai
 */

#pragma once
#include "dev/dtb/tree.h"

bool
plic_init_from_dtb(const struct devicetree *tree,
                   const struct devicetree_node *node);
