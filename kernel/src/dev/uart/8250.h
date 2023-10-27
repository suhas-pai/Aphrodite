/*
 * kernel/dev/uart/8250.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include "sys/port.h"

bool
uart8250_init(port_t base,
              uint32_t baudrate,
              uint32_t in_freq,
              uint8_t reg_width,
              uint8_t reg_shift);