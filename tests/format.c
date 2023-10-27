/*
 * tests/format.c
 * © suhas pai
 */

#include <assert.h>

#include "lib/format.h"
#include "common.h"

#define test_format_to_buffer(buffer_len, expected, str, ...)                  \
    do {                                                                       \
        int count = 0;                                                         \
        const uint64_t length =                                                \
            format_to_buffer(buffer,                                           \
                             buffer_len,                                       \
                             str "%n",                                         \
                             ##__VA_ARGS__,                                    \
                             &count);                                          \
                                                                               \
                                                                               \
        struct string string = STRING_EMPTY();                                 \
        const uint64_t string_length =                                         \
            format_to_string(&string,                                          \
                             str "%n",                                         \
                             ##__VA_ARGS__,                                    \
                             &count);                                          \
                                                                               \
        check_strings(expected, buffer);                                       \
        check_strings(string.gbuffer.begin, buffer);                           \
                                                                               \
        assert(length == LEN_OF(expected));                                    \
        assert(count == (int)LEN_OF(expected));                                \
        memset(buffer, '\0', countof(buffer));                                 \
                                                                               \
        string_destroy(&string);                                               \
    } while (false)

#define test_format_to_buffer_no_count(buffer_len, expected, str, ...)         \
    do {                                                                       \
        const uint64_t length =                                                \
            format_to_buffer(buffer,                                           \
                             buffer_len,                                       \
                             str,                                              \
                             ##__VA_ARGS__);                                   \
                                                                               \
        check_strings(expected, buffer);                                       \
        assert(length == LEN_OF(expected));                                    \
        memset(buffer, '\0', countof(buffer));                                 \
    } while (false)

void test_format() {
    char buffer[4096] = {0};

    {
        // Test invalid formats.
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wformat"
            format_to_buffer(buffer, countof(buffer), "%");
            format_to_buffer(buffer, countof(buffer), "%y");
            format_to_buffer(buffer, countof(buffer), "%.");
            format_to_buffer(buffer, countof(buffer), "%.4");
            format_to_buffer(buffer, countof(buffer), "%.4l");
            format_to_buffer(buffer, countof(buffer), "%.*", 0);
            format_to_buffer(buffer, countof(buffer), "%.*l", 0);
            format_to_buffer(buffer, countof(buffer), "%5");
            format_to_buffer(buffer, countof(buffer), "%5l");
            format_to_buffer(buffer, countof(buffer), "%5ll");
            format_to_buffer(buffer, countof(buffer), "%l");
            format_to_buffer(buffer, countof(buffer), "%*", 0);
        #pragma GCC diagnostic pop
    }

    test_format_to_buffer(countof(buffer), "test", "test");
    test_format_to_buffer(countof(buffer), " test", " %s", "test");
    test_format_to_buffer(countof(buffer), " test ", " %s ", "test");
    test_format_to_buffer(countof(buffer),
                          " test test ",
                          " %s %s ",
                          "test",
                          "test");

    test_format_to_buffer(countof(buffer), "5", "%d", 5);
    test_format_to_buffer(countof(buffer), " 42141 ", " %d ", 42141);
    test_format_to_buffer(countof(buffer), "    3", "%5d", 3);
    test_format_to_buffer(countof(buffer), "   -3", "%5d", -3);
    test_format_to_buffer(countof(buffer), "3    ", "%-5d", 3);
    test_format_to_buffer(countof(buffer), "-3   ", "%-5d", -3);
    test_format_to_buffer(countof(buffer), "-0003", "%05d", -3);
    test_format_to_buffer(countof(buffer), "00003", "%05d", 3);
    test_format_to_buffer(countof(buffer), "    3", "%5u", 3);
    test_format_to_buffer(countof(buffer), "00003", "%05u", 3);
    test_format_to_buffer(countof(buffer), "00003", "%.5u", 3);
    test_format_to_buffer(countof(buffer), "H", "%.1s", "Hi");
    test_format_to_buffer(countof(buffer), "   Hi", "%5s", "Hi");
    test_format_to_buffer(countof(buffer), "Hi   ", "%-5s", "Hi");
    test_format_to_buffer(countof(buffer), "    H", "%5c", 'H');
    test_format_to_buffer(countof(buffer), "H    ", "%-5c", 'H');
    test_format_to_buffer(countof(buffer), "f", "%x", 15);
    test_format_to_buffer(countof(buffer), "F", "%X", 15);
    test_format_to_buffer(countof(buffer), "0xf", "%#x", 15);
    test_format_to_buffer(countof(buffer), "0XF", "%#X", 15);
    test_format_to_buffer(countof(buffer), "0000f", "%05x", 15);
    test_format_to_buffer(countof(buffer), " 0000f", " %05x", 15);
    test_format_to_buffer(countof(buffer), " 0000F", " %05X", 15);
    test_format_to_buffer(countof(buffer), "0000F", "%05X", 15);
    test_format_to_buffer(countof(buffer), "0x00f", "%#05x", 15);
    test_format_to_buffer(countof(buffer), "0X00F", "%#05X", 15);
    test_format_to_buffer(countof(buffer), "7", "%o", 7);
    test_format_to_buffer(countof(buffer), "07", "%#o", 7);
    test_format_to_buffer(countof(buffer), "    7", "%5o", 7);
    test_format_to_buffer(countof(buffer), "   07", "%#5o", 7);
    test_format_to_buffer(countof(buffer), "h", "%c", 'h');
    test_format_to_buffer(countof(buffer), "    h", "%5c", 'h');
    test_format_to_buffer(countof(buffer), "Hello", "%s", "Hello");
    test_format_to_buffer(countof(buffer), " Hello ", " %s ", "Hello");
    test_format_to_buffer(countof(buffer), "(null)", "%s", (char *)NULL);
    test_format_to_buffer(countof(buffer), "(null)", "%5s", (char *)NULL);
    test_format_to_buffer(countof(buffer), "(nil)", "%p", NULL);
    test_format_to_buffer(countof(buffer), "(nil)", "%5p", NULL);
    test_format_to_buffer(countof(buffer), "0x1", "%p", (void *)0x1);
    test_format_to_buffer(countof(buffer), "0xff", "0x%x", 0xff);
    test_format_to_buffer(countof(buffer), " 0005", "%5.4d", 5);
    test_format_to_buffer(countof(buffer), "00000005", "%5.8d", 5);
    test_format_to_buffer(countof(buffer), "     ", "%5.0d", 0);
    test_format_to_buffer(countof(buffer), "-4", "% d", -4);
    test_format_to_buffer(countof(buffer), " 4", "% d",  4);
    test_format_to_buffer(sizeof(buffer), "+4", "%+d", 4);
    test_format_to_buffer(sizeof(buffer), "-4", "%+d", -4);
    test_format_to_buffer(sizeof(buffer), "Hel", "%.*s", 3, "Hello");
    test_format_to_buffer(sizeof(buffer), " Hel", " %.*s", 3, "Hello");

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
    test_format_to_buffer(countof(buffer), "H    ", "%-05c", 'H');
    test_format_to_buffer(sizeof(buffer), "a", "%+x", 10);
    test_format_to_buffer(sizeof(buffer), "A", "%+X", 10);
    test_format_to_buffer(sizeof(buffer), "fffffff6", "%+x", -10);
    test_format_to_buffer(sizeof(buffer), "FFFFFFF6", "%+X", -10);
    test_format_to_buffer_no_count(countof(buffer), "%", "%  %",  4);
    test_format_to_buffer_no_count(countof(buffer), "", "%", 0, "");
    test_format_to_buffer_no_count(countof(buffer), "", "%0", 0, "");
    test_format_to_buffer_no_count(countof(buffer), "", "%#", 0, "");
    test_format_to_buffer_no_count(countof(buffer), "", "%#0", 0, "");
    test_format_to_buffer_no_count(countof(buffer), "", "%-0", 0, "");
    test_format_to_buffer_no_count(countof(buffer), "", "%-#0", 0, "");
    test_format_to_buffer_no_count(countof(buffer), "", "%-#0.", 0, "");
    test_format_to_buffer_no_count(countof(buffer), "", "%-#+0.", 0, "");
    test_format_to_buffer_no_count(countof(buffer), "", "%+", 0, "");
    test_format_to_buffer_no_count(countof(buffer), "", "%0.", 0, "");
    test_format_to_buffer_no_count(countof(buffer), "", "%0.*", 0, "");
    test_format_to_buffer_no_count(countof(buffer), "", "%.0", 0, "");
    test_format_to_buffer_no_count(countof(buffer), "", "%*.0", 0, "");
#pragma GCC diagnostic pop

    const char buffer2[] = "Hello, There";
    test_format_to_buffer(countof(buffer), "Hel", "%.*s", 3, buffer2);
}