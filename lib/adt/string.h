/*
 * lib/adt/string.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/growable_buffer.h"
#include "lib/adt/string_view.h"

struct string {
    struct growable_buffer gbuffer;
};

#define STRING_EMPTY() ((struct string){ .gbuffer = GBUFFER_INIT() })
#define STRING_STATIC(cstr) \
    ((struct string){ \
        .gbuffer = GBUFFER_FROM_PTR(cstr, LEN_OF(cstr)) \
    })

#define STRING_FMT SV_FMT
#define STRING_FMT_ARGS(string) SV_FMT_ARGS(string_to_sv(string))

struct string string_alloc(struct string_view sv);

struct string *string_append_char(struct string *string, char ch, uint32_t amt);
struct string *string_append_sv(struct string *string, struct string_view sv);

__printf_format(2, 3)
struct string *
string_append_format(struct string *string, const char *fmt, ...);

struct string *
string_append(struct string *string, const struct string *append);

char string_front(struct string string);
char string_back(struct string string);
uint64_t string_length(struct string string);

struct string *string_remove_index(struct string *string, uint32_t index);
struct string *string_remove_range(struct string *string, struct range range);

int64_t string_find_char(struct string *string, char ch);
int64_t string_find_sv(struct string *string, struct string_view sv);
int64_t string_find_string(struct string *string, const struct string *ch);

struct string_view string_to_sv(struct string string);
void string_destroy(struct string *string);

