/*
 * kernel/src/sys/mmio.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "lib/assert.h"

uint8_t mmio_read_8(volatile const void *ptr);
uint16_t mmio_read_16(volatile const void *ptr);
uint32_t mmio_read_32(volatile const void *ptr);
uint64_t mmio_read_64(volatile const void *ptr);

void mmio_write_8(volatile void *ptr, uint8_t value);
void mmio_write_16(volatile void *ptr, uint16_t value);
void mmio_write_32(volatile void *ptr, uint32_t value);
void mmio_write_64(volatile void *ptr, uint64_t value);

#define mmio_read(ptr) ({ \
    __auto_type __result__ = (typeof(*(ptr)))0; \
    switch (sizeof(*(ptr))) { \
        case sizeof(uint8_t): \
            __result__ = mmio_read_8(ptr); \
            break; \
        case sizeof(uint16_t): \
            __result__ = mmio_read_16(ptr); \
            break; \
        case sizeof(uint32_t): \
            __result__ = mmio_read_32(ptr); \
            break; \
        case sizeof(uint64_t): \
            __result__ = mmio_read_64(ptr); \
            break; \
        default: \
            verify_not_reached(); \
    } \
    __result__; \
})

#define mmio_write(ptr, value) ({ \
    switch (sizeof(*(ptr))) { \
        case sizeof(uint8_t): \
            mmio_write_8((ptr), (uint8_t)(value)); \
            break; \
        case sizeof(uint16_t): \
            mmio_write_16((ptr), (uint16_t)(value)); \
            break; \
        case sizeof(uint32_t): \
            mmio_write_32((ptr), (uint32_t)(value)); \
            break; \
        case sizeof(uint64_t): \
            mmio_write_64((ptr), (uint64_t)(value)); \
            break; \
        default: \
            verify_not_reached(); \
    } \
})
