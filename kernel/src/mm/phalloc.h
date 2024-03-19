/*
 * kernel/src/mm/phalloc.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PHALLOC_MAX 65536

void phalloc_init();
bool phalloc_initialized();

void phalloc_free(uint64_t phys);

uint64_t phalloc(uint32_t size);
uint64_t phalloc_size(uint32_t size, uint32_t *size_out);

uint64_t phrealloc(uint64_t buffer, uint32_t size);
