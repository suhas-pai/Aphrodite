/*
 * kernel/include/dev/scsi/swap.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

void scsi_swap_data(void *data, uint32_t size);
