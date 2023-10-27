/*
 * src/kernel/sanitizers/undefined.c
 * © suhas pai
 */

#include "cpu/util.h"
#include "dev/printk.h"
#include "lib/align.h"

#include "ubsan.h"

static const char *const type_check_kind_list[] = {
    "load of",
    "store to",
    "reference binding to",
    "member access within",
    "member call on",
    "constructor call on",
    "downcast of",
    "downcast of",
    "upcast of",
    "cast to virtual base of",
    "_Nonnull binding to",
    "dynamic operation on"
};

void
__ubsan_handle_type_mismatch_v1(struct type_mismatch_info_v1 *const info,
                                const uint64_t pointer)
{
    if (pointer == 0) {
        printk(LOGLEVEL_ERROR,
               "ubsan: [" SOURCE_LOCATION_FMT "] %s null pointer of type %s\n",
               SOURCE_LOCATION_FMT_ARGS(&info->location),
               type_check_kind_list[info->type_check_kind],
               info->type->name);
        cpu_halt();
    }

    const uint64_t alignment = 1ull << info->log_alignment;
    if (has_align(pointer, alignment)) {
        printk(LOGLEVEL_ERROR,
               "ubsan: [" SOURCE_LOCATION_FMT "] %" PRIu64 " address with "
               "insufficient space for an object of type %s\n",
               SOURCE_LOCATION_FMT_ARGS(&info->location),
               pointer,
               info->type->name);
    } else {
        // We ignore misaligned accesses for now.
    }
}

void
__ubsan_handle_type_mismatch_v1_abort(struct type_mismatch_info_v1 *const info,
                                      const uint64_t pointer)
{
    __ubsan_handle_type_mismatch_v1(info, pointer);
    cpu_halt();
}

void
__ubsan_handle_pointer_overflow(struct pointer_overflow_info *const info,
                                const uint64_t base,
                                const uint64_t offset)
{
    enum error_kind error_kind = ERROR_KIND_NONE;
    if (base == 0 && offset == 0) {
        error_kind = ERROR_KIND_NullPointerUse;
    } else if (base == 0) {
        error_kind = ERROR_KIND_NullptrWithNonZeroOffset;
    } else if (offset == 0) {
        error_kind = ERROR_KIND_NullptrAfterNonZeroOffset;
    } else {
        error_kind = ERROR_KIND_PointerOverflow;
    }

    switch (error_kind) {
        case ERROR_KIND_NONE:
            verify_not_reached();
        case ERROR_KIND_NullptrWithOffset:
            printk(LOGLEVEL_ERROR,
                   "ubsan: [" SOURCE_LOCATION_FMT "] applying an offset of "
                   "zero to null pointer\n",
                   SOURCE_LOCATION_FMT_ARGS(&info->location));
            break;
        case ERROR_KIND_NullptrWithNonZeroOffset:
            printk(LOGLEVEL_ERROR,
                   "ubsan: [" SOURCE_LOCATION_FMT "] applying non-zero offset "
                   "of %" PRIu64 " to null pointer\n",
                   SOURCE_LOCATION_FMT_ARGS(&info->location),
                   offset);
            break;
        case ERROR_KIND_NullptrAfterNonZeroOffset:
            printk(LOGLEVEL_ERROR,
                   "ubsan: [" SOURCE_LOCATION_FMT "] applying a non-zero "
                   "offset of %" PRIu64 " produced null pointer\n",
                   SOURCE_LOCATION_FMT_ARGS(&info->location),
                   offset);
            break;
        case ERROR_KIND_PointerOverflow:
            printk(LOGLEVEL_ERROR,
                   "ubsan: [" SOURCE_LOCATION_FMT "] applying an offset "
                   "of %" PRIu64 " to %" PRIu64 " overflowed to %" PRIu64 "\n",
                   SOURCE_LOCATION_FMT_ARGS(&info->location),
                   offset,
                   base,
                   base + offset);
            break;
        case ERROR_KIND_GenericUB:
            printk(LOGLEVEL_ERROR,
                   "ubsan: [" SOURCE_LOCATION_FMT "] generic "
                   "undefined-behavior with offset of %" PRIu64 " to %" PRIu64
                   " overflowed to %" PRIu64 "\n",
                   SOURCE_LOCATION_FMT_ARGS(&info->location),
                   offset,
                   base,
                   base + offset);
            break;
        case ERROR_KIND_NullPointerUse:
            if (offset != 0) {
                printk(LOGLEVEL_ERROR,
                       "ubsan: [" SOURCE_LOCATION_FMT "] null pointer use with "
                       "offset of %" PRIu64 " to %" PRIu64 " overflowed to "
                       "%" PRIu64 "\n",
                       SOURCE_LOCATION_FMT_ARGS(&info->location),
                       offset,
                       base,
                       base + offset);
            }

            break;
        case ERROR_KIND_NullPointerUseWithNullability:
        case ERROR_KIND_MisalignedPointerUse:
        case ERROR_KIND_AlignmentAssumption:
        case ERROR_KIND_InsufficientObjectSize:
        case ERROR_KIND_SignedIntegerOverflow:
        case ERROR_KIND_UnsignedIntegerOverflow:
        case ERROR_KIND_IntegerDivideByZero:
        case ERROR_KIND_FloatDivideByZero:
        case ERROR_KIND_InvalidBuiltin:
        case ERROR_KIND_InvalidObjCCast:
        case ERROR_KIND_ImplicitUnsignedIntegerTruncation:
        case ERROR_KIND_ImplicitSignedIntegerTruncation:
        case ERROR_KIND_ImplicitIntegerSignChange:
        case ERROR_KIND_ImplicitSignedIntegerTruncationOrSignChange:
        case ERROR_KIND_InvalidShiftBase:
        case ERROR_KIND_InvalidShiftExponent:
        case ERROR_KIND_OutOfBoundsIndex:
        case ERROR_KIND_UnreachableCall:
        case ERROR_KIND_MissingReturn:
        case ERROR_KIND_NonPositiveVLAIndex:
        case ERROR_KIND_FloatCastOverflow:
        case ERROR_KIND_InvalidBoolLoad:
        case ERROR_KIND_InvalidEnumLoad:
        case ERROR_KIND_FunctionTypeMismatch:
        case ERROR_KIND_InvalidNullReturn:
        case ERROR_KIND_InvalidNullReturnWithNullability:
        case ERROR_KIND_InvalidNullArgument:
        case ERROR_KIND_InvalidNullArgumentWithNullability:
        case ERROR_KIND_DynamicTypeMismatch:
        case ERROR_KIND_CFIBadType:
            verify_not_reached();
    }
}

void
__ubsan_handle_pointer_overflow_abort(struct pointer_overflow_info *const info,
                                      const uint64_t base,
                                      const uint64_t offset)
{
    __ubsan_handle_pointer_overflow(info, base, offset);
    cpu_halt();
}

void
__ubsan_handle_shift_out_of_bounds(struct shift_out_of_bounds_info *const info,
                                   const uint64_t lhs_value,
                                   const uint64_t rhs_value)
{
    if (typedesc_is_signed_int(info->rhs) && (int64_t)rhs_value < 0) {
        printk(LOGLEVEL_ERROR,
               "ubsan: [" SOURCE_LOCATION_FMT "] shift value %" PRIu64 " is "
               "negative\n",
               SOURCE_LOCATION_FMT_ARGS(&info->location),
               rhs_value);
    } else if (rhs_value >= typedesc_get_int_bit_width(info->lhs)) {
        printk(LOGLEVEL_ERROR,
               "ubsan: [" SOURCE_LOCATION_FMT "] shift value %" PRIu64 " is "
               "too large for type %s\n",
               SOURCE_LOCATION_FMT_ARGS(&info->location),
               rhs_value,
               info->lhs->name);
    } else if (typedesc_is_signed_int(info->lhs) && (int64_t)lhs_value < 0) {
        printk(LOGLEVEL_ERROR,
               "ubsan: [" SOURCE_LOCATION_FMT "] left-shift of negative "
               "value %" PRIu64 "\n",
               SOURCE_LOCATION_FMT_ARGS(&info->location),
               lhs_value);
    } else {
        printk(LOGLEVEL_ERROR,
               "ubsan: [" SOURCE_LOCATION_FMT "] left-shift of %" PRIu64 " "
               "by %" PRIu64 " places cannot be represented in type %s\n",
               SOURCE_LOCATION_FMT_ARGS(&info->location),
               lhs_value,
               rhs_value,
               info->lhs->name);
    }
}

void
__ubsan_handle_shift_out_of_bounds_abort(
    struct shift_out_of_bounds_info *const info,
    const uint64_t lhs_value,
    const uint64_t rhs_value)
{
    __ubsan_handle_shift_out_of_bounds(info, lhs_value, rhs_value);
    cpu_halt();
}

void __ubsan_handle_nonnull_arg(struct nonnull_arg_info *const info) {
    printk(LOGLEVEL_ERROR,
           "ubsan: [" SOURCE_LOCATION_FMT "] null pointer passed to argument "
           "at index %" PRId32 ", which is declared to never be null\n",
           SOURCE_LOCATION_FMT_ARGS(&info->location),
           (int32_t)info->arg_index);
}

void
__ubsan_handle_nonnull_arg_abort(struct nonnull_arg_info *const info)
{
    __ubsan_handle_nonnull_arg(info);
    cpu_halt();
}

void
__ubsan_handle_sub_overflow(struct overflow_info *const info,
                            const uint64_t lhs,
                            const uint64_t rhs)
{
    printk(LOGLEVEL_ERROR,
           "ubsan: [" SOURCE_LOCATION_FMT "] %s integer subtraction of %" PRIu64
           " and %" PRIu64 " cannot be represented in type %s\n",
           SOURCE_LOCATION_FMT_ARGS(&info->location),
           typedesc_is_signed_int(info->type) ? "signed" : "unsigned",
           lhs,
           rhs,
           info->type->name);
}

void
__ubsan_handle_add_overflow(struct overflow_info *const info,
                            const uint64_t lhs,
                            const uint64_t rhs)
{
    printk(LOGLEVEL_ERROR,
           "ubsan: [" SOURCE_LOCATION_FMT "] %s integer addition "
           "of %" PRIu64 "and %" PRIu64 " cannot be represented in type %s\n",
           SOURCE_LOCATION_FMT_ARGS(&info->location),
           typedesc_is_signed_int(info->type) ? "signed" : "unsigned",
           lhs,
           rhs,
           info->type->name);
}

void
__ubsan_handle_mul_overflow(struct overflow_info *const info,
                            const uint64_t lhs,
                            const uint64_t rhs)
{
    printk(LOGLEVEL_ERROR,
           "ubsan: [" SOURCE_LOCATION_FMT "] %s integer multiplication "
           "of %" PRIu64 " and %" PRIu64 " cannot be represented in type %s\n",
           SOURCE_LOCATION_FMT_ARGS(&info->location),
           typedesc_is_signed_int(info->type) ? "signed" : "unsigned",
           lhs,
           rhs,
           info->type->name);
}

void
__ubsan_handle_add_overflow_abort(struct overflow_info *const info,
                                  const uint64_t lhs,
                                  const uint64_t rhs)
{
    __ubsan_handle_add_overflow(info, lhs, rhs);
    cpu_halt();
}

void
__ubsan_handle_sub_overflow_abort(struct overflow_info *const info,
                                  const uint64_t lhs,
                                  const uint64_t rhs)
{
    __ubsan_handle_sub_overflow(info, lhs, rhs);
    cpu_halt();
}

void
__ubsan_handle_mul_overflow_abort(struct overflow_info *const info,
                                  const uint64_t lhs,
                                  const uint64_t rhs)
{
    __ubsan_handle_mul_overflow(info, lhs, rhs);
    cpu_halt();
}

void
__ubsan_handle_negate_overflow(struct overflow_info *const info,
                               const uint64_t value)
{
    const bool is_signed = typedesc_is_signed_int(info->type);
    printk(LOGLEVEL_ERROR,
           "ubsan: [" SOURCE_LOCATION_FMT "] %s integer negation of %" PRIu64
           " cannot be represented in type %s\n",
           SOURCE_LOCATION_FMT_ARGS(&info->location),
           is_signed ? "signed" : "unsigned",
           value,
           info->type->name);

    if (is_signed) {
        printk(LOGLEVEL_ERROR,
               "ubsan: cast value to an unsigned type to work around this "
               "issue.\n");
    }
}

void
__ubsan_handle_negate_overflow_abort(struct overflow_info *const info,
                                     const uint64_t value)
{
    __ubsan_handle_negate_overflow(info, value);
    cpu_halt();
}

void
__ubsan_handle_divrem_overflow(struct overflow_info *const info,
                               const uint64_t lhs,
                               const uint64_t rhs)
{
    if (typedesc_is_signed_int(info->type) && (int64_t)rhs == -1) {
        printk(LOGLEVEL_ERROR,
               "ubsan: [" SOURCE_LOCATION_FMT "] signed integer division "
               "of %" PRIu64 "by %" PRIu64 " cannot be represented in "
               "type %s\n",
               SOURCE_LOCATION_FMT_ARGS(&info->location),
               lhs,
               rhs,
               info->type->name);
    } else {
        printk(LOGLEVEL_ERROR,
               "ubsan: [" SOURCE_LOCATION_FMT "] division by zero with "
               "type %s\n",
               SOURCE_LOCATION_FMT_ARGS(&info->location),
               info->type->name);
    }
}

void
__ubsan_handle_divrem_overflow_abort(struct overflow_info *const info,
                                     const uint64_t lhs,
                                     const uint64_t rhs)
{
    __ubsan_handle_divrem_overflow(info, lhs, rhs);
    cpu_halt();
}

void
__ubsan_handle_out_of_bounds(struct out_of_bounds_info *const info,
                             const uint64_t index)
{
    printk(LOGLEVEL_ERROR,
           "ubsan: [" SOURCE_LOCATION_FMT "] index %" PRIu64 " is out of "
           "bounds for type %s\n",
           SOURCE_LOCATION_FMT_ARGS(&info->location),
           index,
           info->array_type->name);
}

void
__ubsan_handle_out_of_bounds_abort(struct out_of_bounds_info *const info,
                                   const uint64_t index)
{
    __ubsan_handle_out_of_bounds(info, index);
    cpu_halt();
}

void
__ubsan_handle_builtin_unreachable(struct unreachable_info *const info) {
    printk(LOGLEVEL_ERROR,
           "ubsan: [" SOURCE_LOCATION_FMT "] Reached the unreachable\n",
           SOURCE_LOCATION_FMT_ARGS(&info->location));
}

void __ubsan_handle_missing_return(struct unreachable_info *const info) {
    printk(LOGLEVEL_ERROR,
           "ubsan: [" SOURCE_LOCATION_FMT "] function doesn't return a value\n",
           SOURCE_LOCATION_FMT_ARGS(&info->location));
}

void
__ubsan_handle_vla_bound_not_positive(struct vla_bound_info *const info,
                                      const uint64_t bound)
{
    printk(LOGLEVEL_ERROR,
           "ubsan: [" SOURCE_LOCATION_FMT "] Variable length array bound is "
           "not positive: %" PRIu64 "\n",
           SOURCE_LOCATION_FMT_ARGS(&info->location),
           bound);
}

void
__ubsan_handle_float_cast_overflow(struct float_cast_overflow_info *const info,
                                   const uint64_t value)
{
    (void)value;
    printk(LOGLEVEL_ERROR,
           "ubsan: [" SOURCE_LOCATION_FMT "] Float Value cannot be represented "
           "in type %s\n",
           SOURCE_LOCATION_FMT_ARGS(&info->location),
           info->type->name);
}

void
__ubsan_handle_load_invalid_value(struct invalid_value_info *const info,
                                  const uint64_t value)
{
    printk(LOGLEVEL_ERROR,
           "ubsan: [" SOURCE_LOCATION_FMT "] load of value %" PRIu64 " into "
           "type %s which is not a valid value\n",
           SOURCE_LOCATION_FMT_ARGS(&info->location),
           value,
           info->type->name);
}

void
__ubsan_handle_type_mismatch(struct type_mismatch_info *const info,
                             const uint64_t value)
{
    printk(LOGLEVEL_ERROR,
           "ubsan: [" SOURCE_LOCATION_FMT "] load of bvalue %" PRIu64 " into "
           "type %s which cannot be represented\n",
           SOURCE_LOCATION_FMT_ARGS(&info->location),
           value,
           info->type->name);
}

void
__ubsan_handle_implicit_conversion(struct implicit_conversion_info *const info,
                                   const uint64_t value)
{
    if (typedesc_is_signed_int(info->to_type)) {
        printk(LOGLEVEL_ERROR,
               "ubsan: [" SOURCE_LOCATION_FMT "] implicit conversion from "
               "type %s of value %" PRIu64 " (%s, %" PRIu8 "-bit) to type %s "
               "changed value to %" PRId64 " (%s, %" PRIu8 "-bit)\n",
               SOURCE_LOCATION_FMT_ARGS(&info->location),
               info->from_type->name,
               value,
               typedesc_is_signed_int(info->from_type) ? "signed" : "unsigned",
               typedesc_get_int_bit_width(info->from_type),
               info->to_type->name,
               (int64_t)value,
               typedesc_is_signed_int(info->to_type) ? "signed" : "unsigned",
               typedesc_get_int_bit_width(info->to_type));
    } else {
        printk(LOGLEVEL_ERROR,
               "ubsan: [" SOURCE_LOCATION_FMT "] implicit conversion from "
               "type %s of value %" PRIu64 " (%s, %" PRIu8 "-bit) to type %s "
               "changed value to %" PRIu64 " (%s, %" PRIu8 "-bit)\n",
               SOURCE_LOCATION_FMT_ARGS(&info->location),
               info->from_type->name,
               value,
               typedesc_is_signed_int(info->from_type) ? "signed" : "unsigned",
               typedesc_get_int_bit_width(info->from_type),
               info->to_type->name,
               value,
               typedesc_is_signed_int(info->to_type) ? "signed" : "unsigned",
               typedesc_get_int_bit_width(info->to_type));
    }
}

void
__ubsan_handle_implicit_conversion_abort(
    struct implicit_conversion_info *const info,
    const uint64_t value)
{
    __ubsan_handle_implicit_conversion(info, value);
    cpu_halt();
}

void __ubsan_handle_nonnull_return_v1(struct nonnull_return_info *const info) {
    printk(LOGLEVEL_ERROR,
           "ubsan: [" SOURCE_LOCATION_FMT "] null pointer returned from "
           "function\n",
           SOURCE_LOCATION_FMT_ARGS(&info->location));
}

void
__ubsan_handle_nonnull_return_v1_abort(struct nonnull_return_info *const info) {
    __ubsan_handle_nonnull_return_v1(info);
    cpu_halt();
}

void
__ubsan_handle_invalid_builtin(struct invalid_builtin_info *const info,
                               const enum builtin_check_kind kind)
{
    printk(LOGLEVEL_ERROR,
           "ubsan: [" SOURCE_LOCATION_FMT "] passing zero to %s is not "
           "allowed\n",
           SOURCE_LOCATION_FMT_ARGS(&info->location),
           kind == BUILTIN_CHECK_KIND_CLZ_PASSED_ZERO ?
            "__builtin_ctz" : "__builtin_clz");
}

void
__ubsan_handle_function_type_mismatch(
    struct function_type_mismatch_info *const info,
    const uint64_t value)
{
    printk(LOGLEVEL_ERROR,
           "ubsan: [" SOURCE_LOCATION_FMT "] call to function at address %p "
           "through pointer with incorrect function type %s\n",
           SOURCE_LOCATION_FMT_ARGS(&info->location),
           (void *)value,
           info->type->name);
}