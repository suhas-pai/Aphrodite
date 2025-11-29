/*
 * lib/path.h
 * © suhas pai
 */

#pragma once
#include "adt/string_view.h"

struct string_view path_cstr_dirname(const char *c_str);
struct string_view path_cstr_basename(const char *c_str);
struct string_view path_cstr_extension(const char *c_str);
struct string_view path_cstr_filename(const char *c_str);

bool path_cstr_is_relative(const char *path);
struct string_view path_cstr_filename_without_extension(const char *path);

struct string_view path_sv_dirname(struct string_view path);
struct string_view path_sv_basename(struct string_view path);
struct string_view path_sv_extension(struct string_view path);
struct string_view path_sv_filename(struct string_view path);

bool path_sv_is_relative(struct string_view path);

struct string_view
path_sv_filename_without_extension(struct string_view path);

struct string_view path_cstr_get_first_component(const char *c_str);
struct string_view path_sv_get_first_component(struct string_view path);

struct string_view
path_cstr_get_next_component(const char *c_str,
                             struct string_view prev_component);

struct string_view
path_sv_get_next_component(struct string_view path,
                           struct string_view prev_component);

#define path_cstr_foreach_component(the_path, iter) \
    const auto h_var(path) = (the_path); \
    for (struct string_view iter = path_cstr_get_first_component(h_var(path)); \
         !sv_is_empty(iter); \
         iter = path_cstr_get_next_component(h_var(path), iter))

#define path_sv_foreach_component(the_path, iter) \
    const auto h_var(path) = (the_path); \
    for (struct string_view iter = path_sv_get_first_component(h_var(path)); \
         !sv_is_empty(iter); \
         iter = path_sv_get_next_component(h_var(path), iter))

bool path_component_is_redirect(struct string_view component);
