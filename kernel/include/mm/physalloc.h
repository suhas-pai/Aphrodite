/*
 * kernel/src/mm/physalloc.h
 * © suhas pai
 */

#pragma once
#include "mm/mm_types.h"

#define PHALLOC_MAX 65536

void physalloc_init();
bool physalloc_initialized();

void physalloc_free(uint64_t phys);

uint64_t physalloc(uint32_t size);
uint64_t physalloc_size(uint32_t size, uint32_t *size_out);

uint64_t physrealloc(uint64_t buffer, uint32_t size);
