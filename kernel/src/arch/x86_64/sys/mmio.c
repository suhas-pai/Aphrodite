/*
 * kernel/arch/x86_64/sys/mmio.c
 * © suhas pai
 */

#include "sys/mmio.h"

__optimize(3) uint8_t mmio_read_8(volatile const void *const ptr) {
    return *(volatile const uint8_t *)ptr;
}

__optimize(3) uint16_t mmio_read_16(volatile const void *const ptr) {
    return *(volatile const uint16_t *)ptr;
}

__optimize(3) uint32_t mmio_read_32(volatile const void *const ptr) {
    return *(volatile const uint32_t *)ptr;
}

__optimize(3) uint64_t mmio_read_64(volatile const void *const ptr) {
    return *(volatile const uint64_t *)ptr;
}

__optimize(3) void mmio_write_8(volatile void *const ptr, const uint8_t value) {
    *(volatile uint8_t *)ptr = value;
}

__optimize(3)
void mmio_write_16(volatile void *const ptr, const uint16_t value) {
    *(volatile uint16_t *)ptr = value;
}

__optimize(3)
void mmio_write_32(volatile void *const ptr, const uint32_t value) {
    *(volatile uint32_t *)ptr = value;
}

__optimize(3)
void mmio_write_64(volatile void *const ptr, const uint64_t value) {
    *(volatile uint64_t *)ptr = value;
}