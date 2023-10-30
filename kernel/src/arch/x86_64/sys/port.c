
/*
 * kernel/src/arch/x86_64/sys/port.c
 * © suhas pai
 */

#include "lib/macros.h"
#include "sys/port.h"

__optimize(3) uint8_t port_in8(const port_t port) {
    uint8_t result = 0;
    asm volatile (
        "in %1, %0\n\t"
        : "=a" (result)
        : "Nd" (port)
        : "memory");

    return result;
}

__optimize(3) uint16_t port_in16(const port_t port) {
    uint16_t result = 0;
    asm volatile ("in %1, %0" : "=a" (result) : "Nd" (port));

    return result;
}

__optimize(3) uint32_t port_in32(const port_t port) {
    uint32_t result = 0;
    asm volatile ("in %1, %0" : "=a" (result) : "Nd" (port));

    return result;
}

__optimize(3) void port_out8(const port_t port, const uint8_t value) {
    asm volatile ("out %1, %0" : : "Nd" (port), "a" (value));
}

__optimize(3) void port_out16(const port_t port, const uint16_t value) {
    asm volatile ("out %1, %0" : : "Nd" (port), "a" (value));
}

__optimize(3) void port_out32(const port_t port, const uint32_t value) {
    asm volatile ("out %1, %0" : : "Nd" (port), "a" (value));
}