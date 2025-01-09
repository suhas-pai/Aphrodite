/*
 * kernel/include/time/time.h
 * © suhas pai
 */

#pragma once
#include "lib/time.h"

nsec_t nsec_since_boot();
void stall_for_usec(usec_t usec);
