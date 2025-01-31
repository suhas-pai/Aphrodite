/*
 * kernel/include/sys/uacpi/uacpi_libc.h
 * © suhas pai
 */

#pragma once

#include <lib/format.h>
#include <lib/string.h>

#define uacpi_memcpy memcpy
#define uacpi_memset memset
#define uacpi_memcmp memcmp
#define uacpi_strcmp strcmp
#define uacpi_memmove memmove
#define uacpi_strnlen strnlen
#define uacpi_strlen strlen
#define uacpi_snprintf snprintf
#define uacpi_vsnprintf vsnprintf
