/*
 * lib/size.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/string_view.h"
#include "lib/inttypes.h"

#define kib(amount) ((1ull << 10) * (amount))
#define mib(amount) ((1ull << 20) * (amount))
#define gib(amount) ((1ull << 30) * (amount))
#define tib(amount) ((1ull << 40) * (amount))
#define pib(amount) ((1ull << 50) * (amount))
#define eib(amount) ((1ull << 60) * (amount))

#define C_STR_BYTE "Byte"
#define C_STR_KIB "Kibibyte"
#define C_STR_MIB "Mebibyte"
#define C_STR_GIB "Gibibyte"
#define C_STR_TIB "Tebibyte"
#define C_STR_PIB "Pebibyte"
#define C_STR_EIB "Exbiibyte"

#define C_STR_BYTE_ABBREV "Byte"
#define C_STR_KIB_ABBREV "Kib"
#define C_STR_MIB_ABBREV "Mib"
#define C_STR_GIB_ABBREV "Gib"
#define C_STR_TIB_ABBREV "Tib"
#define C_STR_PIB_ABBREV "Pib"
#define C_STR_PIB_ABBREV "Pib"
#define C_STR_EIB_ABBREV "Eib"

#define BYTE_SV SV_STATIC(C_STR_BYTE)
#define KIB_SV SV_STATIC(C_STR_KIB)
#define MIB_SV SV_STATIC(C_STR_MIB)
#define GIB_SV SV_STATIC(C_STR_GIB)
#define TIB_SV SV_STATIC(C_STR_TIB)
#define PIB_SV SV_STATIC(C_STR_PIB)
#define EIB_SV SV_STATIC(C_STR_EIB)

#define BYTE_SV_ABBREV SV_STATIC(C_STR_BYTE_ABBREV)
#define KIB_SV_ABBREV SV_STATIC(C_STR_KIB_ABBREV)
#define MIB_SV_ABBREV SV_STATIC(C_STR_MIB_ABBREV)
#define GIB_SV_ABBREV SV_STATIC(C_STR_GIB_ABBREV)
#define TIB_SV_ABBREV SV_STATIC(C_STR_TIB_ABBREV)
#define PIB_SV_ABBREV SV_STATIC(C_STR_PIB_ABBREV)
#define EIB_SV_ABBREV SV_STATIC(C_STR_EIB_ABBREV)

enum size_unit_kind {
    UNIT_KIND_BYTE = 1,
    UNIT_KIND_KIB = kib(1),
    UNIT_KIND_MIB = mib(1),
    UNIT_KIND_GIB = gib(1),
    UNIT_KIND_TIB = tib(1),
    UNIT_KIND_PIB = pib(1),
    UNIT_KIND_EIB = eib(1)
};

#define SIZE_UNIT_FMT "%" PRIu64 ".%" PRIu64 " " SV_FMT
#define SIZE_UNIT_FMT_ARGS(size) \
    (uint64_t)((size) / size_to_units(size)), \
    (uint64_t)(((size) % size_to_units(size)) / 1000), \
    SV_FMT_ARGS(size_units_to_sv(size_to_units(size)))

#define SIZE_UNIT_FMT_ARGS_ABBREV(size) \
    (uint64_t)(size) / size_to_units(size), \
    (uint64_t)(((size) % size_to_units(size)) / 1000), \
    SV_FMT_ARGS(size_units_to_sv_abbrev(size_to_units(size)))

struct string_view size_units_to_sv(enum size_unit_kind kind);
struct string_view size_units_to_sv_abbrev(enum size_unit_kind kind);

enum size_unit_kind size_to_units(uint64_t size);
