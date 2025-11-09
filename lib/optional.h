/*
 * lib/optional.h
 * © suhas pai
 */

#pragma once
#include "macros.h"

#define optional_of_carr(arr, index, null) ({ \
    (index_in_bounds(index, countof(arr)) ? arr[index] : (null)); \
})
