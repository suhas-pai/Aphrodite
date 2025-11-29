/*
 * tests/path.c
 * © suhas pai
 */

#include <lib/adt/string_view.h>
#include <lib/path.h>

#include "tests/common.h"

struct path_info {
    struct string_view path;
    struct string_view dirname;
    struct string_view basename;
    struct string_view extension;
    struct string_view filename_wo_ext;
    struct string_view filename_with_ext;
    struct string_view *components;
};

static const struct path_info g_test_paths[] = {
    {
        .path = "",
        .dirname = SV_STATIC("."),
        .basename = SV_STATIC(""),
        .extension = SV_EMPTY(),
        .filename_wo_ext = SV_STATIC(""),
        .filename_with_ext = SV_STATIC(""),
        .components = (struct string_view[]){}
    },
    {
        .path = SV_STATIC("/"),
        .dirname = SV_STATIC("/"),
        .basename = SV_STATIC("/"),
        .extension = SV_EMPTY(),
        .filename_wo_ext = SV_STATIC(""),
        .filename_with_ext = SV_STATIC(""),
        .components = (struct string_view[]){
            SV_STATIC("/")
        }
    },
    {
        .path = SV_STATIC("/usr/local/bin/test.exe"),
        .dirname = SV_STATIC("/usr/local/bin"),
        .basename = SV_STATIC("test.exe"),
        .extension = SV_STATIC("exe"),
        .filename_wo_ext = SV_STATIC("test"),
        .filename_with_ext = SV_STATIC("test.exe"),
        .components = (struct string_view[]){
            SV_STATIC("/"),
            SV_STATIC("usr"),
            SV_STATIC("local"),
            SV_STATIC("bin"),
            SV_STATIC("test.exe"),
        }
    },
    {
        .path = SV_STATIC("relative/path/to/file.txt"),
        .dirname = SV_STATIC("relative/path/to"),
        .basename = SV_STATIC("file.txt"),
        .extension = SV_STATIC("txt"),
        .filename_wo_ext = SV_STATIC("file"),
        .filename_with_ext = SV_STATIC("file.txt"),
        .components = (struct string_view[]){
            SV_STATIC("relative"),
            SV_STATIC("path"),
            SV_STATIC("to"),
            SV_STATIC("file.txt"),
        }
    }
};


int test_path() {
    carr_foreach(g_test_paths, ti) {
        const struct string_view dirname_cstr =
            path_cstr_dirname(ti->path.begin);

        const struct string_view dirname_sv = path_sv_dirname(ti->path);

        check_sv(dirname_cstr, ti->dirname);
        check_sv(dirname_sv, ti->dirname);

        const struct string_view basename_cstr =
            path_cstr_basename(ti->path.begin);

        const struct string_view basename_sv = path_sv_basename(ti->path);

        check_sv(basename_cstr, ti->basename);
        check_sv(basename_sv, ti->basename);

        const struct string_view filename_with_ext_cstr =
            path_cstr_filename(ti->path.begin);
        const struct string_view filename_with_ext_sv =
            path_sv_filename(ti->path);

        check_sv(filename_with_ext_cstr, ti->filename_with_ext);
        check_sv(filename_with_ext_sv, ti->filename_with_ext);

        const struct string_view extension_cstr =
            path_cstr_extension(ti->path.begin);

        const struct string_view extension_sv = path_sv_extension(ti->path);

        check_sv(extension_cstr, ti->extension);
        check_sv(extension_sv, ti->extension);

        const struct string_view filename_wo_ext_cstr =
            path_cstr_filename_without_extension(ti->path.begin);
        const struct string_view filename_wo_ext_sv =
            path_sv_filename_without_extension(ti->path);

        check_sv(filename_wo_ext_cstr, ti->filename_wo_ext);
        check_sv(filename_wo_ext_sv, ti->filename_wo_ext);

        uint32_t comp_idx = 0;
        path_cstr_foreach_component(ti->path.begin, comp) {
            check_sv(comp, ti->components[comp_idx]);
            comp_idx++;
        }

        comp_idx = 0;
        path_sv_foreach_component(ti->path, comp) {
            check_sv(comp, ti->components[comp_idx]);
            comp_idx++;
        }
    }

    printf("path: All tests passed!\n");
    return 0;
}
