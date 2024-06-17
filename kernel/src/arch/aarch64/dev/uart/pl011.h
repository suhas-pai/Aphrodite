/*
 * kernel/src/arch/aarch64/dev/uart/pl011.h
 * © suhas pai
 */

#pragma once
#include "sys/pio.h"

void
pl011_init(port_t base,
           const uint32_t baudrate,
           const uint32_t data_bits,
           const uint32_t stop_bits);
