/*
 * sys/port.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

#if defined(__x86_64__)
    typedef uint16_t port_t;
#elif defined(__aarch64__) || defined(__riscv64)
    typedef volatile void *port_t;
#endif /* defined(__x86_64__) || defined(__aarch64__) */

uint8_t port_in8(port_t port);
uint16_t port_in16(port_t port);
uint32_t port_in32(port_t port);

#if defined(__aarch64__) || defined(__riscv64)
    uint64_t port_in64(port_t port);
#endif /* defined(__aarch64__) || defined(__riscv64) */

void port_out8(port_t port, uint8_t value);
void port_out16(port_t port, uint16_t value);
void port_out32(port_t port, uint32_t value);

#if defined(__aarch64__) || defined(__riscv64)
    void port_out64(port_t port, uint64_t value);
#endif /* defined(__aarch64__) || defined(__riscv64) */