/*
 * kernel/src/arch/loongarch64/port.c
 * © suhas pai
 */

#include "lib/macros.h"
#include "sys/pio.h"

__debug_optimize(3) uint8_t pio_read8(const port_t port) {
    return *(volatile uint8_t *)port;
}

__debug_optimize(3) uint16_t pio_read16(const port_t port) {
    return *(volatile uint16_t *)port;
}

__debug_optimize(3) uint32_t pio_read32(const port_t port) {
    return *(volatile uint32_t *)port;
}

__debug_optimize(3) uint64_t pio_read64(const port_t port) {
    return *(volatile uint64_t *)port;
}

__debug_optimize(3) void pio_write8(const port_t port, const uint8_t value) {
    *(volatile uint8_t *)port = value;
}

__debug_optimize(3) void pio_write16(const port_t port, const uint16_t value) {
    *(volatile uint16_t *)port = value;
}

__debug_optimize(3) void pio_write32(const port_t port, const uint32_t value) {
    *(volatile uint32_t *)port = value;
}

__debug_optimize(3) void pio_write64(const port_t port, const uint64_t value) {
    *(volatile uint64_t *)port = value;
}
