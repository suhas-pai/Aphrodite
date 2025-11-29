/*
 * lib/path.c
 * © suhas pai
 */

#include "adt/string_view.h"
#include "util.h"
#include "path.h"

__debug_optimize(3)
struct string_view path_cstr_dirname(const char *const c_str) {
    return path_sv_dirname(sv_create(c_str));
}

__debug_optimize(3)
struct string_view path_cstr_basename(const char *const c_str) {
    return path_sv_basename(sv_create(c_str));
}

__debug_optimize(3)
struct string_view path_cstr_extension(const char *const c_str) {
    return path_sv_extension(sv_create(c_str));
}

__debug_optimize(3)
struct string_view path_cstr_filename(const char *const c_str) {
    return path_sv_filename(sv_create(c_str));
}

__debug_optimize(3)
struct string_view path_sv_dirname(const struct string_view path) {
    if (__builtin_expect(sv_is_empty(path), 1)) {
        return SV_STATIC(".");
    }

    const int64_t last_slash_idx = sv_find_char_rev(path, '/');
    if (last_slash_idx == SV_NPOS) {
        return SV_STATIC(".");
    }

    if (last_slash_idx == 0) {
        return SV_STATIC("/");
    }

    return sv_substring_length(path, 0, last_slash_idx);
}

__debug_optimize(3)
struct string_view path_sv_basename(const struct string_view path) {
    if (__builtin_expect(sv_is_empty(path), 1)) {
        return SV_EMPTY();
    }

    const int64_t last_slash_idx = sv_find_char_rev(path, '/');
    if (last_slash_idx == SV_NPOS) {
        return path;
    }

    if (last_slash_idx == 0) {
        return sv_substring_upto(path, 1);
    }

    const uint32_t index = (uint32_t)(last_slash_idx + 1);
    return sv_substring_from(path, index);
}

__debug_optimize(3)
struct string_view path_sv_filename(struct string_view path) {
    const struct string_view basename = path_sv_basename(path);
    if (sv_equals(basename, SV_STATIC("/"))) {
        return SV_EMPTY();
    }

    return basename;
}

__debug_optimize(3)
struct string_view path_sv_extension(const struct string_view path) {
    const struct string_view basename = path_sv_basename(path);
    const int64_t last_dot_idx = sv_find_char_rev(basename, '.');

    if (last_dot_idx == SV_NPOS) {
        return SV_EMPTY();
    }

    const uint32_t index = (uint32_t)(last_dot_idx + 1);
    return sv_substring_from(basename, index);
}

__debug_optimize(3) bool path_cstr_is_relative(const char *const path) {
    return *path != '/';
}

__debug_optimize(3) struct string_view
path_cstr_filename_without_extension(const char *const path) {
    return path_sv_filename_without_extension(sv_create(path));
}

__debug_optimize(3) bool path_sv_is_relative(const struct string_view path) {
    return sv_front(path) != '/';
}

__debug_optimize(3) struct string_view
path_sv_filename_without_extension(const struct string_view path) {
    const struct string_view filename = path_sv_filename(path);
    const int64_t last_dot_index = sv_find_char_rev(filename, '.');

    if (last_dot_index == SV_NPOS) {
        return filename;
    }

    return sv_substring_upto(filename, last_dot_index);
}

struct string_view path_cstr_get_first_component(const char *const path) {
    if (*path == '\0') {
        return SV_EMPTY();
    }

    if (*path == '/') {
        return sv_create_nocheck(path, 1);
    }

    const char *const first_slash = strchr(path, '/');
    if (first_slash == nullptr) {
        return sv_create(path);
    }

    const uint64_t first_slash_idx = distance(path, first_slash);
    return sv_create_nocheck(path, (uint32_t)first_slash_idx);
}

__debug_optimize(3)
struct string_view path_sv_get_first_component(const struct string_view path) {
    if (__builtin_expect(sv_is_empty(path), 1)) {
        return SV_EMPTY();
    }

    if (sv_front(path) == '/') {
        return sv_substring_length(path, 0, 1);
    }

    const int64_t first_slash_idx = sv_find_char(path, '/');
    if (first_slash_idx == SV_NPOS) {
        return path;
    }

    return sv_substring_upto(path, (uint32_t)first_slash_idx);
}

__debug_optimize(3) struct string_view
path_cstr_get_next_component(const char *const path,
                             const struct string_view prev_component)
{
    if (__builtin_expect(*path == '\0' || sv_is_empty(prev_component), 1)) {
        return SV_EMPTY();
    }

    const uint32_t prev_end =
        distance(path, prev_component.begin) + prev_component.length;

    if (path[prev_end] == '\0') {
        return SV_EMPTY();
    }

    const char *const remaining_path = path + prev_end;
    const char *const next_slash = strchr(remaining_path, '/');

    if (next_slash == nullptr) {
        return sv_create(remaining_path);
    }

    const uint64_t next_slash_idx = distance(remaining_path, next_slash);
    return sv_create_nocheck(remaining_path, (uint32_t)next_slash_idx);
}


__debug_optimize(3) struct string_view
path_sv_get_next_component(const struct string_view path,
                           const struct string_view prev_component)
{
    if (__builtin_expect(sv_is_empty(path) || sv_is_empty(prev_component), 1)) {
        return SV_EMPTY();
    }

    const uint32_t start_index =
        (uint32_t)
           (distance(path.begin, prev_component.begin) + prev_component.length);

    if (!index_in_bounds(start_index, path.length)) {
        return SV_EMPTY();
    }

    const struct string_view remaining_path =
        sv_substring_from(path, start_index);

    const int64_t next_slash_idx = sv_find_char(remaining_path, '/');
    if (next_slash_idx == SV_NPOS) {
        return remaining_path;
    }

    return sv_substring_upto(remaining_path, (uint32_t)next_slash_idx);
}

__debug_optimize(3)
bool path_component_is_redirect(struct string_view component) {
    return sv_equals_c_str(component, "..");
}
