/*
 * kernel/include/dev/dtb/init.h
 * © suhas pai
 */

#pragma once

#include "dev/dtb/driver.h"
#include "dev/dtb/node.h"

void dtb_parse_main_tree();

bool
dtb_init_nodes_for_driver(const struct dtb_driver *const driver,
                          const struct devicetree *const tree,
                          const struct devicetree_node *const node);
