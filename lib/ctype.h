/*
 * lib/ctype.h
 * © suhas pai
 */

#pragma once

#if defined(BUILD_KERNEL)
    #define C_TYPE char
#else
    #define C_TYPE int
#endif

int isalnum(C_TYPE c);
int isalpha(C_TYPE c);
int iscntrl(C_TYPE c);
int isdigit(C_TYPE c);
int isgraph(C_TYPE c);
int islower(C_TYPE c);
int isprint(C_TYPE c);
int ispunct(C_TYPE c);
int isspace(C_TYPE c);
int isupper(C_TYPE c);
int isxdigit(C_TYPE c);

int tolower(C_TYPE c);
int toupper(C_TYPE c);