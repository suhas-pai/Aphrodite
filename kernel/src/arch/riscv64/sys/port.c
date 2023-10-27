/*
 * kernel/arch/riscv64/sys/port.c
 * © suhas pai
 */

#include "sys/mmio.h"
#include "sys/port.h"

__optimize(3) uint8_t port_in8(const port_t port) {
    return mmio_read_8(port);
}

__optimize(3) uint16_t port_in16(const port_t port) {
    return mmio_read_16(port);
}

__optimize(3) uint32_t port_in32(const port_t port) {
    return mmio_read_32(port);
}

__optimize(3) uint64_t port_in64(const port_t port) {
    return mmio_read_64(port);
}

__optimize(3) void port_out8(const port_t port, const uint8_t value) {
    mmio_write_8(port, value);
}

__optimize(3) void port_out16(const port_t port, const uint16_t value) {
    mmio_write_16(port, value);
}

__optimize(3) void port_out32(const port_t port, const uint32_t value) {
    mmio_write_32(port, value);
}

__optimize(3) void port_out64(const port_t port, const uint64_t value) {
    mmio_write_64(port, value);
}