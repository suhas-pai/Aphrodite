/*
 * kernel/src/dev/uart/8250.h
 * © suhas pai
 */

#pragma once
#include "sys/pio.h"

bool
uart8250_init(port_t base,
              uint32_t baudrate,
              uint32_t in_freq,
              uint8_t reg_width,
              uint8_t reg_shift);
