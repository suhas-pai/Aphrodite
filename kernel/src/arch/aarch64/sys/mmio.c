/*
 * kernel/src/arch/aarch64/sys/mmio.c
 * © suhas pai
 */

#include "sys/mmio.h"

__debug_optimize(3) uint8_t mmio_read_8(volatile const void *const ptr) {
    return *(volatile const uint8_t *)ptr;
}

__debug_optimize(3) uint16_t mmio_read_16(volatile const void *const ptr) {
    return *(volatile const uint16_t *)ptr;
}

__debug_optimize(3) uint32_t mmio_read_32(volatile const void *const ptr) {
    return *(volatile const uint32_t *)ptr;
}

__debug_optimize(3) uint64_t mmio_read_64(volatile const void *const ptr) {
    return *(volatile const uint64_t *)ptr;
}

uint64_t mmio_read_size(volatile const void *const ptr, const size_t size) {
    switch (size) {
        case sizeof(uint8_t):
            return mmio_read_8(ptr);
        case sizeof(uint16_t):
            return mmio_read_16(ptr);
        case sizeof(uint32_t):
            return mmio_read_32(ptr);
        case sizeof(uint64_t):
            return mmio_read_64(ptr);
    }

    verify_not_reached();
}

__debug_optimize(3)
void mmio_write_8(volatile void *const ptr, const uint8_t value) {
    *(volatile uint8_t *)ptr = value;
}

__debug_optimize(3)
void mmio_write_16(volatile void *const ptr, const uint16_t value) {
    *(volatile uint16_t *)ptr = value;
}

__debug_optimize(3)
void mmio_write_32(volatile void *const ptr, const uint32_t value) {
    *(volatile uint32_t *)ptr = value;
}

__debug_optimize(3)
void mmio_write_64(volatile void *const ptr, const uint64_t value) {
    *(volatile uint64_t *)ptr = value;
}

void
mmio_write_size(volatile void *const ptr,
                const size_t size,
                const uint64_t value)
{
    switch (size) {
        case sizeof(uint8_t):
            assert(value <= UINT8_MAX);
            mmio_write_8(ptr, value);

            return;
        case sizeof(uint16_t):
            assert(value <= UINT16_MAX);
            mmio_write_16(ptr, value);

            return;
        case sizeof(uint32_t):
            assert(value <= UINT32_MAX);
            mmio_write_32(ptr, value);

            return;
        case sizeof(uint64_t):
            mmio_write_64(ptr, value);
            return;
    }

    verify_not_reached();
}
