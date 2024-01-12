/*
 * lib/freq.h
 * © suhas pai
 */

#pragma once
#include "adt/string_view.h"

#define khz(amount) (1000ull * (amount))
#define mhz(amount) (khz(1000) * (amount))
#define ghz(amount) (mhz(1000) * (amount))
#define thz(amount) (ghz(1000) * (amount))
#define phz(amount) (thz(1000) * (amount))
#define ehz(amount) (phz(1000) * (amount))

#define C_STR_HZ "Hertz"
#define C_STR_KHZ "Kilohertz"
#define C_STR_MHZ "Megahertz"
#define C_STR_GHZ "Gigahertz"
#define C_STR_THZ "Terahertz"
#define C_STR_PHZ "Petahertz"
#define C_STR_EHZ "Exahertz"

#define C_STR_HZ_ABBREV "Hz"
#define C_STR_KHZ_ABBREV "KHz"
#define C_STR_MHZ_ABBREV "MHz"
#define C_STR_GHZ_ABBREV "GHz"
#define C_STR_THZ_ABBREV "THz"
#define C_STR_PHZ_ABBREV "PHz"
#define C_STR_EHZ_ABBREV "EHz"

#define HZ_SV SV_STATIC(C_STR_HZ)
#define KHZ_SV SV_STATIC(C_STR_KHZ)
#define MHZ_SV SV_STATIC(C_STR_MHZ)
#define GHZ_SV SV_STATIC(C_STR_GHZ)
#define THZ_SV SV_STATIC(C_STR_THZ)
#define PHZ_SV SV_STATIC(C_STR_PHZ)
#define PHZ_SV SV_STATIC(C_STR_PHZ)
#define EHZ_SV SV_STATIC(C_STR_EHZ)

#define HZ_SV_ABBREV SV_STATIC(C_STR_HZ_ABBREV)
#define KHZ_SV_ABBREV SV_STATIC(C_STR_KHZ_ABBREV)
#define MHZ_SV_ABBREV SV_STATIC(C_STR_MHZ_ABBREV)
#define GHZ_SV_ABBREV SV_STATIC(C_STR_GHZ_ABBREV)
#define THZ_SV_ABBREV SV_STATIC(C_STR_THZ_ABBREV)
#define PHZ_SV_ABBREV SV_STATIC(C_STR_PHZ_ABBREV)
#define EHZ_SV_ABBREV SV_STATIC(C_STR_EHZ_ABBREV)

enum freq_unit_kind {
    FREQ_UNIT_KIND_HZ = 1,
    FREQ_UNIT_KIND_KHZ = khz(1),
    FREQ_UNIT_KIND_MHZ = mhz(1),
    FREQ_UNIT_KIND_GHZ = ghz(1),
    FREQ_UNIT_KIND_THZ = thz(1),
    FREQ_UNIT_KIND_PHZ = phz(1),
    FREQ_UNIT_KIND_EHZ = ehz(1)
};

#define FREQ_TO_UNIT_FMT "%" PRIu64 ".%" PRIu64 " " SV_FMT
#define FREQ_TO_UNIT_FMT_ARGS(freq) \
    (freq) / freq_to_units(freq), \
    (freq) % freq_to_units(freq), \
    SV_FMT_ARGS(freq_units_to_sv(freq_to_units(freq)))

#define FREQ_TO_UNIT_FMT_ARGS_ABBREV(freq) \
    (freq) / freq_to_units(freq), \
    (freq) % freq_to_units(freq), \
    SV_FMT_ARGS(freq_units_to_sv_abbrev(freq_to_units(freq)))

struct string_view freq_units_to_sv(enum freq_unit_kind kind);
struct string_view freq_units_to_sv_abbrev(enum freq_unit_kind kind);

enum freq_unit_kind freq_to_units(uint64_t freq);
