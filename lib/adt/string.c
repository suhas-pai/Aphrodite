/*
 * lib/adt/string.c
 * © suhas pai
 */

#include "lib/format.h"
#include "growable_buffer.h"

__debug_optimize(3)
static inline void set_null_terminator(const struct string *const string) {
    /*
     * HACK: We can't officially include the null-terminator as part of the
     * used-size of the gbuffer.
     *
     * Instead, we can set the byte right after used-part of the gbuffer to 0
     * for the null-terminator.
     *
     * prepare_append() is responsible for ensuring we can add one extra byte.
     */

    const struct growable_buffer gbuffer = string->gbuffer;
    ((uint8_t *)gbuffer.begin)[gbuffer.index] = '\0';
}

__debug_optimize(3) struct string string_alloc(const struct string_view sv) {
    struct string result = STRING_NULL();
    string_append_sv(&result, sv);

    return result;
}

__debug_optimize(3) struct string string_copy(const struct string string) {
    const struct string result = { .gbuffer = gbuffer_copy(string.gbuffer) };
    return result;
}

__debug_optimize(3) struct string string_format(const char *const fmt, ...) {
    va_list list;
    va_start(list, fmt);

    struct string result = string_vformat(fmt, list);

    va_end(list);
    return result;
}

__debug_optimize(3)
struct string string_vformat(const char *const fmt, va_list list) {
    struct string result = STRING_NULL();
    vformat_to_string(&result, fmt, list);

    return result;
}

__debug_optimize(3) static inline
bool prepare_append(struct string *const string, const uint32_t length) {
    return gbuffer_ensure_can_add_capacity(&string->gbuffer, length + 1);
}

__debug_optimize(3) struct string *
string_append_char(struct string *const string,
                   const char ch,
                   const uint32_t amount)
{
    if (__builtin_expect(!prepare_append(string, amount), 0)) {
        return NULL;
    }

    if (__builtin_expect(!gbuffer_append_byte(&string->gbuffer, ch, amount), 0))
    {
        return NULL;
    }

    set_null_terminator(string);
    return string;
}

__debug_optimize(3) struct string *
string_append_sv(struct string *const string, const struct string_view sv) {
    if (__builtin_expect(!prepare_append(string, sv.length), 0)) {
        gbuffer_destroy(&string->gbuffer);
        return NULL;
    }

    if (__builtin_expect(!gbuffer_append_sv(&string->gbuffer, sv), 0)) {
        gbuffer_destroy(&string->gbuffer);
        return NULL;
    }

    set_null_terminator(string);
    return string;
}

__debug_optimize(3) struct string *
string_append_format(struct string *const string, const char *const fmt, ...) {
    va_list list;
    va_start(list, fmt);

    vformat_to_string(string, fmt, list);

    va_end(list);
    return string;
}

__debug_optimize(3) struct string *
string_append_vformat(struct string *const string,
                      const char *const fmt,
                      va_list list)
{
    vformat_to_string(string, fmt, list);
    return string;
}

__debug_optimize(3) struct string *
string_append(struct string *const string, const struct string *const append) {
    if (__builtin_expect(!prepare_append(string, string_length(*string)), 0)) {
        return NULL;
    }

    if (__builtin_expect(
            gbuffer_append_gbuffer_data(&string->gbuffer, &append->gbuffer), 0))
    {
        return NULL;
    }

    set_null_terminator(string);
    return string;
}

__debug_optimize(3) char string_front(const struct string string) {
    if (__builtin_expect(!gbuffer_empty(string.gbuffer), 1)) {
        return ((uint8_t *)string.gbuffer.begin)[0];
    }

    verify_not_reached();
}

__debug_optimize(3) char string_back(const struct string string) {
    const uint32_t length = string_length(string);
    if (__builtin_expect(length != 0, 1)) {
        return ((uint8_t *)string.gbuffer.begin)[length - 1];
    }

    verify_not_reached();
}

__debug_optimize(3) uint32_t string_length(const struct string string) {
    return gbuffer_used_size(string.gbuffer);
}

__debug_optimize(3)
void string_reserve(struct string *string, uint32_t capacity) {
    gbuffer_ensure_can_add_capacity(&string->gbuffer, capacity);
}

__debug_optimize(3) struct string *
string_remove_index(struct string *const string, const uint32_t index) {
    gbuffer_remove_index(&string->gbuffer, index);
    return string;
}

__debug_optimize(3) struct string *
string_remove_range(struct string *const string, const struct range range) {
    gbuffer_remove_range(&string->gbuffer, range);
    return string;
}

__debug_optimize(3)
int64_t string_find_char(struct string *const string, char ch) {
    char *const result = strchr(string->gbuffer.begin, ch);
    if (result != NULL) {
        return ((int64_t)result - (int64_t)string->gbuffer.begin);
    }

    return -1;
}

__debug_optimize(3) int64_t
string_find_sv(struct string *const string, const struct string_view sv) {
    const uint32_t string_len = string_length(*string);
    if (string_len == 0 || sv.length == 0) {
        return -1;
    }

    if (sv.length > string_len) {
        return -1;
    }

    const char *ptr = string->gbuffer.begin;
    for (uint32_t i = 0; i <= (string_len - sv.length); i++, ptr++) {
        if (strncmp(ptr, sv.begin, sv.length) == 0) {
            return i;
        }
    }

    return -1;
}

__debug_optimize(3) int64_t
string_find_string(struct string *const string, const struct string *const find)
{
    return string_find_sv(string, string_to_sv(*find));
}

__debug_optimize(3)
struct string_view string_to_sv(const struct string string) {
    const uint32_t length = string_length(string);
    if (length == 0) {
        return SV_EMPTY();
    }

    return sv_create_nocheck(string.gbuffer.begin, length);
}

__debug_optimize(3) const char *string_to_cstr(const struct string string) {
    assert(string.gbuffer.begin != NULL);
    return string.gbuffer.begin;
}

__debug_optimize(3) void string_destroy(struct string *const string) {
    gbuffer_destroy(&string->gbuffer);
}

