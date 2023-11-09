/*
 * kernel/src/sys/pio.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

#if defined(__x86_64__)
    typedef uint16_t port_t;
#elif defined(__aarch64__) || defined(__riscv64)
    typedef volatile void *port_t;
#endif /* defined(__x86_64__) || defined(__aarch64__) */

uint8_t pio_read8(port_t port);
uint16_t pio_read16(port_t port);
uint32_t pio_read32(port_t port);

#if defined(__aarch64__) || defined(__riscv64)
    uint64_t pio_read64(port_t port);
#endif /* defined(__aarch64__) || defined(__riscv64) */

void pio_write8(port_t port, uint8_t value);
void pio_write16(port_t port, uint16_t value);
void pio_write32(port_t port, uint32_t value);

#if defined(__aarch64__) || defined(__riscv64)
    void pio_write64(port_t port, uint64_t value);
#endif /* defined(__aarch64__) || defined(__riscv64) */