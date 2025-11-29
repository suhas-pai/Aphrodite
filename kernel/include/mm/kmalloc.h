/*
 * kernel/include/mm/kmalloc.h
 * © suhas pai
 */

#pragma once
#include <lib/macros.h>

#define KMALLOC_MAX (uint32_t)32768
#define kmalloc_arr(type, count) \
    ((type *)kmalloc(ckd_mul_assert(sizeof(type), (uint32_t)(count))))

void kmalloc_check_slabs();

void kmalloc_init();
bool kmalloc_initialized();

void kfree(void *buffer);

__malloclike __malloc_dealloc(kfree, 1) __alloc_size(1)
void *kmalloc(uint32_t size);

__malloclike __malloc_dealloc(kfree, 1)
void *kmalloc_size(uint32_t size, uint32_t *size_out);

__alloc_size(2) void *krealloc(void *buffer, uint32_t size);
