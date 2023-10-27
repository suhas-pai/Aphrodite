/*
 * lib/strftime.c
 * © suhas pai
 */

#include "adt/mutable_buffer.h"

#include "parse_strftime.h"
#include "strftime.h"

static uint64_t
time_format_to_string_sv_callback(
    const struct strftime_spec_info *const spec_info,
    void *const cb_info,
    const struct string_view sv,
    bool *const should_cont_out)
{
    (void)spec_info;

    struct mutable_buffer *const mbuffer = (struct mutable_buffer *)cb_info;
    const uint64_t result = mbuffer_append_sv(mbuffer, sv);

    // We have filled up the buffer if result isn't equal to sv-length
    if (result != sv.length || mbuffer_full(*mbuffer)) {
        *should_cont_out = false;
    }

    return result;
}

uint64_t
time_format_to_string_buffer(char *const buffer_in,
                             const uint64_t buffer_cap,
                             const char *const format,
                             const struct tm *const tm)
{
    if (buffer_cap == 0) {
        return 0;
    }

    struct mutable_buffer mbuffer =
        mbuffer_open(buffer_in, /*used=*/0, buffer_cap);
    const uint64_t result =
        parse_strftime_format(time_format_to_string_sv_callback,
                              &mbuffer,
                              format,
                              tm);

    *(uint8_t *)mbuffer_current_ptr(mbuffer) = '\0';
    return result;
}

static uint64_t
time_format_get_length_sv_callback(
    const struct strftime_spec_info *const spec_info,
    void *const info,
    const struct string_view sv,
    bool *const should_continue_out)
{
    (void)spec_info;
    (void)info;
    (void)sv;
    (void)should_continue_out;

    return sv.length;
}

uint64_t
get_length_of_time_format(const char *const format, const struct tm *const tm) {
    const uint64_t result =
        parse_strftime_format(time_format_get_length_sv_callback,
                              /*sv_cb_info=*/NULL,
                              format,
                              tm);
    return result;
}