/*
 * kernel/src/arch/aarch64/sys/irq.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "lib/inttypes.h"

typedef uint16_t irq_number_t;

#define IRQ_NUMBER_FMT "%" PRIu16

