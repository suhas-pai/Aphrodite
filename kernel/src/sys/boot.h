/*
 * kernel/src/sys/boot.h
 * © suhas pai
 */

#pragma once
#include "limine.h"

void boot_init();
void boot_post_early_init();

uint8_t mm_get_memmap_count();
uint8_t mm_get_section_count();

const struct limine_framebuffer_response *boot_get_fb();
const struct limine_smp_response *boot_get_smp();

const void *boot_get_rsdp();
const void *boot_get_dtb();

int64_t boot_get_time();
uint64_t mm_get_full_section_mask();

void boot_merge_usable_memmaps();
void boot_recalculate_pfns();
