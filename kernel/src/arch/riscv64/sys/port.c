/*
 * kernel/src/arch/riscv64/sys/port.c
 * © suhas pai
 */

#include "sys/mmio.h"
#include "sys/pio.h"

__optimize(3) uint8_t pio_read8(const port_t port) {
    return mmio_read_8(port);
}

__optimize(3) uint16_t pio_read16(const port_t port) {
    return mmio_read_16(port);
}

__optimize(3) uint32_t pio_read32(const port_t port) {
    return mmio_read_32(port);
}

__optimize(3) uint64_t pio_read64(const port_t port) {
    return mmio_read_64(port);
}

__optimize(3) void pio_write8(const port_t port, const uint8_t value) {
    mmio_write_8(port, value);
}

__optimize(3) void pio_write16(const port_t port, const uint16_t value) {
    mmio_write_16(port, value);
}

__optimize(3) void pio_write32(const port_t port, const uint32_t value) {
    mmio_write_32(port, value);
}

__optimize(3) void pio_write64(const port_t port, const uint64_t value) {
    mmio_write_64(port, value);
}