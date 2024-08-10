/*
 * lib/format.c
 * © suhas pai
 */

#include "format.h"
#include "parse_printf.h"

__debug_optimize(3) uint32_t
format_to_buffer(char *const buffer,
                 const uint32_t buffer_len,
                 const char *const fmt,
                 ...)
{
    va_list list;
    va_start(list, fmt);

    const uint32_t result = vformat_to_buffer(buffer, buffer_len, fmt, list);

    va_end(list);
    return result;
}

__debug_optimize(3) static uint32_t
buffer_char_callback(struct printf_spec_info *const spec_info,
                     void *const cb_info,
                     const char ch,
                     const uint32_t amount,
                     bool *const cont_out)
{
    (void)spec_info;

    struct mutable_buffer *const mbuffer = cb_info;
    const uint32_t appended = mbuffer_append_byte(mbuffer, ch, amount);

    if (appended != amount) {
        *cont_out = false;
    }

    return appended;
}

__debug_optimize(3) static uint32_t
buffer_sv_callback(struct printf_spec_info *const spec_info,
                   void *const cb_info,
                   const struct string_view sv,
                   bool *const cont_out)
{
    (void)spec_info;

    struct mutable_buffer *const mbuffer = cb_info;
    const uint32_t appended = mbuffer_append_sv(mbuffer, sv);

    if (appended != sv.length) {
        *cont_out = false;
    }

    return appended;
}

__debug_optimize(3) uint32_t
vformat_to_buffer(char *const buffer,
                  const uint32_t buffer_length,
                  const char *const fmt,
                  va_list list)
{
    struct mutable_buffer mbuffer =
        mbuffer_open(buffer, /*used=*/0, buffer_length);
    const uint32_t result =
        parse_printf(fmt,
                     buffer_char_callback,
                     &mbuffer,
                     buffer_sv_callback,
                     &mbuffer,
                     list);
    return result;
}

__debug_optimize(3) uint32_t
format_to_string(struct string *const string, const char *const fmt, ...) {
    va_list list;
    va_start(list, fmt);

    const uint32_t result = vformat_to_string(string, fmt, list);

    va_end(list);
    return result;
}

__debug_optimize(3) uint32_t
string_char_callback(struct printf_spec_info *const spec_info,
                     void *const cb_info,
                     const char ch,
                     const uint32_t amount,
                     bool *const cont_out)
{
    (void)spec_info;
    if (string_append_char((struct string *)cb_info, ch, amount)) {
        return amount;
    }

    *cont_out = false;
    return 0;
}

__debug_optimize(3) uint32_t
string_sv_callback(struct printf_spec_info *const spec_info,
                   void *const cb_info,
                   const struct string_view sv,
                   bool *const cont_out)
{
    (void)spec_info;
    if (string_append_sv((struct string *)cb_info, sv)) {
        return sv.length;
    }

    *cont_out = false;
    return 0;
}

__debug_optimize(3) uint32_t
vformat_to_string(struct string *const string,
                  const char *const fmt,
                  va_list list)
{
    const uint32_t result =
        parse_printf(fmt,
                     string_char_callback,
                     /*write_char_cb=*/string,
                     string_sv_callback,
                     /*char_cb_info=*/string,
                     list);
    return result;
}