/*
 * kernel/src/cpu/info.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include "limine.h"

struct cpu_info;

extern struct cpu_info g_base_cpu_info;
const struct cpu_info *this_cpu();

struct cpu_info *this_cpu_mut();
struct cpu_info *cpu_add(const struct limine_smp_info *info);

bool cpu_in_bad_state();
