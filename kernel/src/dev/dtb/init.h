/*
 * kernel/src/dev/dtb/init.h
 * © suhas pai
 */

#pragma once

void dtb_parse_main_tree();
void dtb_init();

struct devicetree *dtb_get_tree();