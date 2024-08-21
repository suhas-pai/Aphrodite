/*
 * kernel/src/arch/riscv64/sys/mmio.c
 * © suhas pai
 */

#include "sys/mmio.h"

__debug_optimize(3) static inline void io_br_fence() {
    asm volatile ("fence i,r" ::: "memory");
}

__debug_optimize(3) static inline void io_bw_fence() {
    asm volatile ("fence w,o" ::: "memory");
}

__debug_optimize(3) uint8_t mmio_read_8(volatile const void *const ptr) {
    uint8_t result = 0;
    asm volatile ("lb %0, 0(%1)" : "=r"(result) : "r"(ptr) : "memory");

    io_br_fence();
    return result;
}

__debug_optimize(3) uint16_t mmio_read_16(volatile const void *const ptr) {
    uint16_t result = 0;
    asm volatile ("lh %0, 0(%1)" : "=r"(result) : "r"(ptr) : "memory");

    io_br_fence();
    return result;
}

__debug_optimize(3) uint32_t mmio_read_32(volatile const void *const ptr) {
    uint32_t result = 0;
    asm volatile ("lw %0, 0(%1)" : "=r"(result) : "r"(ptr) : "memory");

    io_br_fence();
    return result;
}

__debug_optimize(3) uint64_t mmio_read_64(volatile const void *const ptr) {
    uint64_t result = 0;
    asm volatile ("ld %0, 0(%1)" : "=r"(result) : "r"(ptr) : "memory");

    io_br_fence();
    return result;
}

__debug_optimize(3)
void mmio_write_8(volatile void *const ptr, const uint8_t value) {
    io_bw_fence();
    asm volatile ("sb %0, 0(%1)" :: "r"(value), "r"(ptr) : "memory");
}

__debug_optimize(3)
void mmio_write_16(volatile void *const ptr, const uint16_t value) {
    io_bw_fence();
    asm volatile ("sh %0, 0(%1)" :: "r"(value), "r"(ptr) : "memory");
}

__debug_optimize(3)
void mmio_write_32(volatile void *const ptr, const uint32_t value) {
    io_bw_fence();
    asm volatile ("sw %0, 0(%1)" :: "r"(value), "r"(ptr) : "memory");
}

__debug_optimize(3)
void mmio_write_64(volatile void *const ptr, const uint64_t value) {
    io_bw_fence();
    asm volatile ("sd %0, 0(%1)" :: "r"(value), "r"(ptr) : "memory");
}