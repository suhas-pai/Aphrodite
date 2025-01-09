/*
 * kernel/include/arch/riscv64/dev/syscon.h
 * © suhas pai
 */

#pragma once
#include "lib/macros.h"

[[noreturn]] void syscon_poweroff();
[[noreturn]] void syscon_reboot();

struct devicetree;
struct devicetree_node;

bool
syscon_init_from_dtb(const struct devicetree *tree,
                     const struct devicetree_node *node);

bool
syscon_init_reboot_dtb(const struct devicetree *tree,
                       const struct devicetree_node *node);

bool
syscon_init_poweroff_dtb(const struct devicetree *tree,
                         const struct devicetree_node *node);