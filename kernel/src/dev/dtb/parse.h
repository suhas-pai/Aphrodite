/*
 * kernel/src/dev/dtb/parse.h
 * © suhas pai
 */

#pragma once
#include "dev/dtb/tree.h"

bool devicetree_parse(struct devicetree *tree, const void *dtb);