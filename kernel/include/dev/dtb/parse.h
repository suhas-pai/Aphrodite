/*
 * kernel/include/dev/dtb/parse.h
 * © suhas pai
 */

#pragma once
#include "tree.h"

bool devicetree_parse(struct devicetree *tree, const void *dtb);
