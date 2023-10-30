/*
 * kernel/src/arch/aarch64/port.c
 * © suhas pai
 */

#include "lib/macros.h"
#include "sys/port.h"

__optimize(3) uint8_t port_in8(const port_t port) {
    return *(volatile uint8_t *)port;
}

__optimize(3) uint16_t port_in16(const port_t port) {
    return *(volatile uint16_t *)port;
}

__optimize(3) uint32_t port_in32(const port_t port) {
    return *(volatile uint32_t *)port;
}

__optimize(3) uint64_t port_in64(const port_t port) {
    return *(volatile uint64_t *)port;
}

__optimize(3) void port_out8(const port_t port, const uint8_t value) {
    *(volatile uint8_t *)port = value;
}

__optimize(3) void port_out16(const port_t port, const uint16_t value) {
    *(volatile uint16_t *)port = value;
}

__optimize(3) void port_out32(const port_t port, const uint32_t value) {
    *(volatile uint32_t *)port = value;
}

__optimize(3) void port_out64(const port_t port, const uint64_t value) {
    *(volatile uint64_t *)port = value;
}