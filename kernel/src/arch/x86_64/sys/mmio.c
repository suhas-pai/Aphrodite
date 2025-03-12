/*
 * kernel/src/arch/x86_64/sys/mmio.c
 * © suhas pai
 */

#include "sys/mmio.h"

__debug_optimize(3) uint8_t mmio_read_8(volatile const void *const ptr) {
    uint8_t x = 0;
    asm volatile ("movb (%1), %0" : "=a" (x) : "r" (ptr) : "memory");

    return x;
}

__debug_optimize(3) uint16_t mmio_read_16(volatile const void *const ptr) {
    uint16_t x = 0;
    asm volatile ("movw (%1), %0" : "=a" (x) : "r" (ptr) : "memory");

    return x;
}

__debug_optimize(3) uint32_t mmio_read_32(volatile const void *const ptr) {
    uint32_t x = 0;
    asm volatile ("movl (%1), %0" : "=a" (x) : "r" (ptr) : "memory");

    return x;
}

__debug_optimize(3) uint64_t mmio_read_64(volatile const void *const ptr) {
    uint64_t x = 0;
    asm volatile ("movq (%1), %0" : "=a" (x) : "r" (ptr) : "memory");

    return x;
}

__debug_optimize(3)
void mmio_write_8(volatile void *const ptr, const uint8_t value) {
    asm volatile ("movb %0, (%1)" :: "a" (value), "r" (ptr) : "memory");
}

__debug_optimize(3)
void mmio_write_16(volatile void *const ptr, const uint16_t value) {
    asm volatile ("movw %0, (%1)" :: "a" (value), "r" (ptr) : "memory");
}

__debug_optimize(3)
void mmio_write_32(volatile void *const ptr, const uint32_t value) {
    asm volatile ("movl %0, (%1)" :: "a" (value), "r" (ptr) : "memory");
}

__debug_optimize(3)
void mmio_write_64(volatile void *const ptr, const uint64_t value) {
    asm volatile ("movq %0, (%1)" :: "a" (value), "r" (ptr) : "memory");
}