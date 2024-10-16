/*
 * lib/size.c
 * © suhas pai
 */

#include "size.h"

__debug_optimize(3)
struct string_view size_units_to_sv(const enum size_unit_kind kind) {
    switch (kind) {
    #define UNIT_KIND_CASE(name) \
        case VAR_CONCAT(UNIT_KIND_, name): \
            return VAR_CONCAT(name, _SV)

        UNIT_KIND_CASE(BYTE);
        UNIT_KIND_CASE(KIB);
        UNIT_KIND_CASE(MIB);
        UNIT_KIND_CASE(GIB);
        UNIT_KIND_CASE(TIB);
        UNIT_KIND_CASE(PIB);
        UNIT_KIND_CASE(EIB);

    #undef UNIT_KIND_CASE
        default:
            verify_not_reached();
    }
}

__debug_optimize(3)
struct string_view size_units_to_sv_abbrev(const enum size_unit_kind kind) {
    switch (kind) {
    #define UNIT_KIND_CASE(name) \
        case VAR_CONCAT(UNIT_KIND_, name): \
            return VAR_CONCAT(name, _SV_ABBREV)

        UNIT_KIND_CASE(BYTE);
        UNIT_KIND_CASE(KIB);
        UNIT_KIND_CASE(MIB);
        UNIT_KIND_CASE(GIB);
        UNIT_KIND_CASE(TIB);
        UNIT_KIND_CASE(PIB);
        UNIT_KIND_CASE(EIB);

    #undef UNIT_KIND_CASE
        default:
            verify_not_reached();
    }
}

__debug_optimize(3) enum size_unit_kind size_to_units(uint64_t size) {
    enum size_unit_kind kind = UNIT_KIND_BYTE;
    while (size >= kib(1)) {
        size /= kib(1);
        kind *= kib(1);
    }

    return kind;
}