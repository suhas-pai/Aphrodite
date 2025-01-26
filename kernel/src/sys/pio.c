/*
 * kernel/src/sys/pio.c
 * © suhas pai
 */

#include <lib/assert.h>
#include "sys/pio.h"

uint64_t pio_read_size(const port_t port, const size_t size) {
    switch (size) {
        case sizeof(uint8_t):
            return pio_read8(port);
        case sizeof(uint16_t):
            return pio_read16(port);
        case sizeof(uint32_t):
            return pio_read32(port);
    #ifdef HAS_64B_PORTS
        case sizeof(uint64_t):
            return pio_read64(port);
    #endif /* defined(HAS_64B_PORTS) */
    }

    verify_not_reached();
}

void
pio_write_size(const port_t port, const size_t size, const uint64_t value) {
    switch (size) {
        case sizeof(uint8_t):
            assert(value <= UINT8_MAX);
            pio_write8(port, value);

            return;
        case sizeof(uint16_t):
            assert(value <= UINT16_MAX);
            pio_write16(port, value);

            return;
        case sizeof(uint32_t):
            assert(value <= UINT32_MAX);
            pio_write32(port, value);

            return;
    #ifdef HAS_64B_PORTS
        case sizeof(uint64_t):
            assert(value <= UINT64_MAX);
            pio_write64(port, value);

            return;
    #endif /* defined(HAS_64B_PORTS) */
    }

    verify_not_reached();
}
