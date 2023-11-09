/*
 * lib/memory.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

uint16_t *memset16(uint16_t *buf, uint64_t count, uint16_t c);
uint32_t *memset32(uint32_t *buf, uint64_t count, uint32_t c);
uint64_t *memset64(uint64_t *buf, uint64_t count, uint64_t c);

bool membuf8_is_all(uint8_t *buf, uint64_t count, uint8_t c);
bool membuf16_is_all(uint16_t *buf, uint64_t count, uint16_t c);
bool membuf32_is_all(uint32_t *buf, uint64_t count, uint32_t c);
bool membuf64_is_all(uint64_t *buf, uint64_t count, uint64_t c);