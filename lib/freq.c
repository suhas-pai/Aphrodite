/*
 * lib/freq.c
 * © suhas pai
 */

#include "lib/macros.h"
#include "freq.h"

__optimize(3)
struct string_view freq_units_to_sv(const enum freq_unit_kind kind) {
    switch (kind) {
    #define UNIT_KIND_CASE(name)                                               \
        case VAR_CONCAT(FREQ_UNIT_KIND_, name):                                \
            return VAR_CONCAT(name, _SV)

        UNIT_KIND_CASE(HZ);
        UNIT_KIND_CASE(KHZ);
        UNIT_KIND_CASE(MHZ);
        UNIT_KIND_CASE(GHZ);
        UNIT_KIND_CASE(THZ);
        UNIT_KIND_CASE(PHZ);
        UNIT_KIND_CASE(EHZ);

    #undef UNIT_KIND_CASE
    }

    verify_not_reached();
}

__optimize(3)
struct string_view freq_units_to_sv_abbrev(const enum freq_unit_kind kind) {
    switch (kind) {
    #define UNIT_KIND_CASE(name)                                               \
        case VAR_CONCAT(FREQ_UNIT_KIND_, name):                                \
            return VAR_CONCAT(name, _SV_ABBREV)

        UNIT_KIND_CASE(HZ);
        UNIT_KIND_CASE(KHZ);
        UNIT_KIND_CASE(MHZ);
        UNIT_KIND_CASE(GHZ);
        UNIT_KIND_CASE(THZ);
        UNIT_KIND_CASE(PHZ);
        UNIT_KIND_CASE(EHZ);

    #undef UNIT_KIND_CASE
    }

    verify_not_reached();
}

__optimize(3) enum freq_unit_kind freq_to_units(uint64_t size) {
    enum freq_unit_kind kind = FREQ_UNIT_KIND_HZ;
    while (size >= khz(1)) {
        size /= khz(1);
        kind *= khz(1);
    }

    return kind;
}