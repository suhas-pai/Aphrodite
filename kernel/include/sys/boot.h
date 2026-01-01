/*
 * kernel/include/sys/boot.h
 * © suhas pai
 */

#pragma once

#include <lib/time.h>
#include "limine.h"

void boot_init();
void boot_post_early_init();

const struct limine_framebuffer_response *boot_get_fb();
const struct limine_mp_response *boot_get_mp();

const void *boot_get_rsdp();
const void *boot_get_dtb();

sec_t boot_get_time();
uint64_t boot_get_slide();

uint64_t mm_get_full_section_mask();
void boot_recalculate_pfns();
