/*
 * kernel/mm/kmalloc.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lib/macros.h"

#define KMALLOC_MAX 8192

void kmalloc_init();
bool kmalloc_initialized();

void kfree(void *buffer);

__malloclike __malloc_dealloc(kfree, 1) __alloc_size(1)
void *kmalloc(uint32_t size);

__malloclike __malloc_dealloc(kfree, 1) __alloc_size(1)
void *kmalloc_size(uint32_t size, uint32_t *size_out);

__alloc_size(2) void *krealloc(void *buffer, uint32_t size);