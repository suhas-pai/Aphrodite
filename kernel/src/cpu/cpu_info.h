/*
 * kernel/src/cpu/info.h
 * © suhas pai
 */

#pragma once
struct cpu_info;

extern struct cpu_info g_base_cpu_info;
const struct cpu_info *get_cpu_info();

struct cpu_info *get_cpu_info_mut();