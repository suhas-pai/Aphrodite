/*
 * lib/util.h
 * © suhas pai
 */

#pragma once
#include "adt/range.h"

bool index_in_bounds(uint64_t index, uint64_t bounds);
bool ordinal_in_bounds(uint64_t ordinal, uint64_t bounds);
bool index_range_in_bounds(struct range range, uint64_t bounds);

const char *get_alphanumeric_upper_string();
const char *get_alphanumeric_lower_string();
