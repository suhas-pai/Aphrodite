/*
 * kernel/src/sys/mmio.h
 * © suhas pai
 */

#pragma once
#include "lib/assert.h"

uint8_t mmio_read_8(volatile const void *ptr);
uint16_t mmio_read_16(volatile const void *ptr);
uint32_t mmio_read_32(volatile const void *ptr);
uint64_t mmio_read_64(volatile const void *ptr);

void mmio_write_8(volatile void *ptr, uint8_t value);
void mmio_write_16(volatile void *ptr, uint16_t value);
void mmio_write_32(volatile void *ptr, uint32_t value);
void mmio_write_64(volatile void *ptr, uint64_t value);

#define mmio_read(ptr) _Generic((ptr), \
    volatile uint8_t *: mmio_read_8, \
    volatile uint16_t *: mmio_read_16, \
    volatile uint32_t *: mmio_read_32, \
    volatile uint64_t *: mmio_read_64, \
    volatile const uint8_t *: mmio_read_8, \
    volatile const uint16_t *: mmio_read_16, \
    volatile const uint32_t *: mmio_read_32, \
    volatile const uint64_t *: mmio_read_64, \
    volatile _Atomic(uint8_t) *: mmio_read_8, \
    volatile _Atomic(uint16_t) *: mmio_read_16, \
    volatile _Atomic(uint32_t) *: mmio_read_32, \
    volatile _Atomic(uint64_t) *: mmio_read_64, \
    volatile const _Atomic(uint8_t) *: mmio_read_8, \
    volatile const _Atomic(uint16_t) *: mmio_read_16, \
    volatile const _Atomic(uint32_t) *: mmio_read_32, \
    volatile const _Atomic(uint64_t) *: mmio_read_64 \
)(ptr)

#define mmio_write(ptr, value) _Generic((ptr), \
    volatile uint8_t *: mmio_write_8, \
    volatile uint16_t *: mmio_write_16, \
    volatile uint32_t *: mmio_write_32, \
    volatile uint64_t *: mmio_write_64, \
    _Atomic uint8_t *: mmio_write_8, \
    _Atomic uint16_t *: mmio_write_16, \
    _Atomic uint32_t *: mmio_write_32, \
    _Atomic uint64_t *: mmio_write_64, \
    volatile _Atomic(uint8_t) *: mmio_write_8, \
    volatile _Atomic(uint16_t) *: mmio_write_16, \
    volatile _Atomic(uint32_t) *: mmio_write_32, \
    volatile _Atomic(uint64_t) *: mmio_write_64 \
)(ptr, value)
