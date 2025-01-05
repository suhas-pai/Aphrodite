/*
 * kernel/src/sys/mmio.c
 * © suhas pai
 */

#include "sys/mmio.h"

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
