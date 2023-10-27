/*
 * kernel/san/ubsan.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lib/macros.h"

struct source_location {
    const char *file;

    uint32_t line;
    uint32_t column;
};

#define SOURCE_LOCATION_FMT "%s:%" PRIu32 ":%" PRIu32
#define SOURCE_LOCATION_FMT_ARGS(loc) (loc)->file, (loc)->line, (loc)->column

enum error_kind {
    ERROR_KIND_NONE,
#define UBSAN_CHECK(Name, SummaryKind, FSanitizeFlagName)                      \
    VAR_CONCAT(ERROR_KIND_, Name),

    #include "ubsan_checks.inc"
#undef UBSAN_CHECK
};

enum type_descriptor_kind {
    TYPE_DESCRIPTOR_KIND_INT,
    TYPE_DESCRIPTOR_KIND_FLOAT,
};

enum type_check_kind {
    TYPE_CHECK_KIND_LOAD,
    TYPE_CHECK_KIND_STORE,
    TYPE_CHECK_KIND_REFERENCE_BINDING,
    TYPE_CHECK_KIND_MEMBER_ACCESS,
    TYPE_CHECK_KIND_MEMBER_CALL,
    TYPE_CHECK_KIND_CONSTRUCTOR_CALL,
    TYPE_CHECK_KIND_DOWNCAST_POINTER,
    TYPE_CHECK_KIND_DOWNCAST_REFERENCE,
    TYPE_CHECK_KIND_UPCAST,
    TYPE_CHECK_KIND_UPCAST_TO_VIRTUAL_BASE,
    TYPE_CHECK_KIND_NONNULL_ASSIGN,
    TYPE_CHECK_KIND_DYNAMIC_OPERATION
};

struct type_descriptor {
    enum type_descriptor_kind kind : 16;
    uint16_t info;

    char name[];
};

static inline
bool typedesc_is_signed_int(const struct type_descriptor *const desc) {
    return (desc->info & 1);
}

static inline
uint8_t typedesc_get_int_bit_width(const struct type_descriptor *const desc) {
    return 1 << (desc->info >> 1);
}

_Static_assert(sizeof(struct type_descriptor) == 4,
               "type_descriptor is not 4 bytes");

struct type_mismatch_info {
    struct source_location location;
    struct type_descriptor *type;

    uint64_t alignment;
    uint8_t type_check_kind;
};

_Static_assert(sizeof(struct type_mismatch_info) ==
                   (sizeof(struct source_location) + sizeof(void *) * 3),
               "type_mismatch_info is not packed");

struct type_mismatch_info_v1 {
    struct source_location location;
    const struct type_descriptor *type;

    uint8_t log_alignment;
    uint8_t type_check_kind;
};

struct unreachable_info {
    struct source_location location;
};

struct pointer_overflow_info {
    struct source_location location;
};

struct shift_out_of_bounds_info {
    struct source_location location;

    const struct type_descriptor *lhs;
    const struct type_descriptor *rhs;
};

struct nonnull_arg_info {
    struct source_location location;
    struct source_location attr_location;

    int arg_index;
};

struct overflow_info {
    struct source_location location;
    struct type_descriptor *type;
};

struct out_of_bounds_info {
    struct source_location location;

    struct type_descriptor *array_type;
    struct type_descriptor *index_type;
};

struct vla_bound_info {
    struct source_location location;
};

struct float_cast_overflow_info {
    struct source_location location;
    struct type_descriptor *type;
};

struct invalid_value_info {
    struct source_location location;
    struct type_descriptor *type;
};

struct function_type_mismatch_info {
    struct source_location location;
    struct type_descriptor *type;
};

struct vptr_null_deref_info {
    struct source_location location;
};

struct invalid_builtin_info {
    struct source_location location;
    struct type_descriptor *type;
};

struct implicit_conversion_info {
    struct source_location location;

    struct type_descriptor *from_type;
    struct type_descriptor *to_type;
};

struct nonnull_return_info {
    struct source_location location;
};

enum builtin_check_kind {
    BUILTIN_CHECK_KIND_CTZ_PASSED_ZERO,
    BUILTIN_CHECK_KIND_CLZ_PASSED_ZERO,
};