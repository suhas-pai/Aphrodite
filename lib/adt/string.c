/*
 * lib/adt/string.c
 * © suhas pai
 */

#include "lib/format.h"
#include "growable_buffer.h"

__optimize(3)
static inline void set_null_terminator(const struct string *const string) {
    /*
     * HACK: We can't officially include the null-terminator as part of the
     * used-size of the gbuffer.
     *
     * Instead, we can set the byte right after used-part of the gbuffer to 0
     * for the null-terminator.
     *
     * NOTE: The caller is responsible for ensuring we can add one extra byte.
     */

    const struct growable_buffer gbuffer = string->gbuffer;
    ((uint8_t *)gbuffer.begin)[gbuffer.index] = '\0';
}

__optimize(3)
struct string string_alloc(const struct string_view sv) {
    struct string result = STRING_EMPTY();
    string_append_sv(&result, sv);

    return result;
}

__optimize(3)
static bool prepare_append(struct string *const string, const uint32_t length) {
    return gbuffer_ensure_can_add_capacity(&string->gbuffer, length + 1);
}

__optimize(3) struct string *
string_append_char(struct string *const string,
                   const char ch,
                   const uint32_t amount)
{
    if (!prepare_append(string, amount)) {
        return NULL;
    }

    if (gbuffer_append_byte(&string->gbuffer, ch, amount) != amount) {
        return NULL;
    }

    set_null_terminator(string);
    return string;
}

__optimize(3) struct string *
string_append_sv(struct string *const string, const struct string_view sv) {
    if (!prepare_append(string, sv.length)) {
        return NULL;
    }

    if (gbuffer_append_sv(&string->gbuffer, sv) != sv.length) {
        return NULL;
    }

    set_null_terminator(string);
    return string;
}

__optimize(3) struct string *
string_append_format(struct string *const string, const char *const fmt, ...) {
    va_list list;
    va_start(list, fmt);

    vformat_to_string(string, fmt, list);

    va_end(list);
    return string;
}

__optimize(3) struct string *
string_append_vformat(struct string *const string,
                      const char *const fmt,
                      va_list list)
{
    vformat_to_string(string, fmt, list);
    return string;
}

__optimize(3) struct string *
string_append(struct string *const string, const struct string *const append) {
    if (!prepare_append(string, string_length(*string))) {
        return NULL;
    }

    if (gbuffer_append_gbuffer_data(&string->gbuffer, &append->gbuffer)) {
        return NULL;
    }

    set_null_terminator(string);
    return string;
}

__optimize(3) char string_front(const struct string string) {
    if (!gbuffer_empty(string.gbuffer)) {
        return ((uint8_t *)string.gbuffer.begin)[0];
    }

    return '\0';
}

__optimize(3) char string_back(const struct string string) {
    const uint64_t length = string_length(string);
    if (length != 0) {
        return ((uint8_t *)string.gbuffer.begin)[length - 1];
    }

    return '\0';
}

__optimize(3) uint64_t string_length(const struct string string) {
    return gbuffer_used_size(string.gbuffer);
}

__optimize(3) struct string *
string_remove_index(struct string *const string, const uint32_t index) {
    gbuffer_remove_index(&string->gbuffer, index);
    return string;
}

__optimize(3) struct string *
string_remove_range(struct string *const string, const struct range range) {
    gbuffer_remove_range(&string->gbuffer, range);
    return string;
}

__optimize(3) int64_t string_find_char(struct string *const string, char ch) {
    char *const result = strchr(string->gbuffer.begin, ch);
    if (result != NULL) {
        return ((int64_t)result - (int64_t)string->gbuffer.begin);
    }

    return -1;
}

__optimize(3) int64_t
string_find_sv(struct string *const string, const struct string_view sv) {
    const uint32_t string_len = string_length(*string);
    if (string_len == 0 || sv.length == 0) {
        return -1;
    }

    if (sv.length > string_len) {
        return -1;
    }

    for (uint32_t i = 0; i <= (string_len - sv.length); i++) {
        if (strncmp(string->gbuffer.begin + i, sv.begin, sv.length) == 0) {
            return i;
        }
    }

    return -1;
}

__optimize(3) int64_t
string_find_string(struct string *const string, const struct string *const find)
{
    return string_find_sv(string, string_to_sv(*find));
}

__optimize(3) struct string_view string_to_sv(const struct string string) {
    const uint64_t length = string_length(string);
    if (length == 0) {
        return SV_EMPTY();
    }

    return sv_create_nocheck(string.gbuffer.begin, length);
}

__optimize(3) void string_destroy(struct string *const string) {
    gbuffer_destroy(&string->gbuffer);
}

