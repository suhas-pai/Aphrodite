/*
 * lib/string.h
 * © suhas pai
 */

#pragma once
#include <stddef.h>

#define c_string_foreach(c_str, name) \
    for (typeof(*c_str) *name = c_str; *name != '\0'; name++)

#if !defined(BUILD_TEST)
    size_t strlen(const char *str);
    size_t strnlen(const char *str, size_t length);

    char *strchr(const char *str, int ch);
    char *strrchr(const char *str, int ch);

    int strcmp(const char *str1, const char *str2);
    int strncmp(const char *str1, const char *str2, size_t length);

    char *strcpy(char *dst, const char *src);
    char *strncpy(char *dst, const char *src, unsigned long n);

    int memcmp(const void *s1, const void *s2, size_t n);
    void *memcpy(void *dst, const void *src, unsigned long n);
    void *memset(void *dst, int val, unsigned long n);
    void *memmove(void *dst, const void *src, unsigned long n);
    void *memchr(const void *ptr, int ch, size_t count);

    void bzero(void *dst, unsigned long n);
    unsigned long int strtoul(const char* str, char** endptr, int base);
#else
    #include <stdlib.h>
    #include <string.h>
#endif /* defined(BUILD_TEST) */

void *memset_all_ones(void *dst, unsigned long n);