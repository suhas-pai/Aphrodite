
/*
 * kernel/src/arch/x86_64/sys/port.c
 * © suhas pai
 */

#include "lib/macros.h"
#include "sys/pio.h"

__optimize(3) uint8_t pio_read8(const port_t port) {
    uint8_t result = 0;
    asm volatile ("in %1, %0\n\t"
                  : "=a" (result)
                  : "Nd" (port)
                  : "memory");

    return result;
}

__optimize(3) uint16_t pio_read16(const port_t port) {
    uint16_t result = 0;
    asm volatile ("in %1, %0" : "=a" (result) : "Nd" (port));

    return result;
}

__optimize(3) uint32_t pio_read32(const port_t port) {
    uint32_t result = 0;
    asm volatile ("in %1, %0" : "=a" (result) : "Nd" (port));

    return result;
}

__optimize(3) void pio_write8(const port_t port, const uint8_t value) {
    asm volatile ("out %1, %0" : : "Nd" (port), "a" (value));
}

__optimize(3) void pio_write16(const port_t port, const uint16_t value) {
    asm volatile ("out %1, %0" : : "Nd" (port), "a" (value));
}

__optimize(3) void pio_write32(const port_t port, const uint32_t value) {
    asm volatile ("out %1, %0" : : "Nd" (port), "a" (value));
}