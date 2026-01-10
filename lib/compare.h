/*
 * lib/compare.h
 * © suhas pai
 */

#pragma once

#define LESS_THAN -1
#define EQUAL_TO 0
#define GREATER_THAN 1

#define is_less_than(a) ((a) < 0)
#define is_greater_than(a) ((a) > 0)
#define is_equal(a) ((a) == 0)
