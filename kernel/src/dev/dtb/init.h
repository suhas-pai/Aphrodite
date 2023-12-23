/*
 * kernel/src/dev/dtb/init.h
 * © suhas pai
 */

#pragma once
#include "driver.h"

void dtb_parse_main_tree();
void dtb_init();

void
dtb_find_nodes_for_driver(const struct dtb_driver *driver,
                          const struct devicetree *tree,
                          struct devicetree_node *node);