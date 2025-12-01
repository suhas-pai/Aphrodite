/*
 * tests/path.c
 * © suhas pai
 */

#include <lib/adt/string_view.h>

#include <lib/path.h>
#include <lib/util.h>

#include "tests/common.h"

struct path_info {
    struct string_view path;
    struct string_view dirname;
    struct string_view basename;
    struct string_view extension;
    struct string_view filename_wo_ext;
    struct string_view filename_with_ext;
    struct string_view *components;

    uint32_t components_count;
    uint32_t non_root_start_index;
};

static const struct path_info g_test_paths[] = {
    {
        .path = "",
        .dirname = SV_STATIC("."),
        .basename = SV_STATIC(""),
        .extension = SV_EMPTY(),
        .filename_wo_ext = SV_STATIC(""),
        .filename_with_ext = SV_STATIC(""),
        .components = (struct string_view[]){},
        .components_count = 0,
        .non_root_start_index = 0,
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
        },
        .components_count = 1,
        .non_root_start_index = 1,
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
        },
        .components_count = 5,
        .non_root_start_index = 1,
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
        },
        .components_count = 4,
        .non_root_start_index = 0,
    }
};

int test_path() {
    bool result = true;
    carr_foreach(g_test_paths, ti) {
        const struct string_view dirname_cstr =
            path_cstr_dirname(ti->path.begin);

        const struct string_view dirname_sv = path_sv_dirname(ti->path);

        check_sv_set_result(dirname_cstr, ti->dirname, result);
        check_sv_set_result(dirname_sv, ti->dirname, result);

        const struct string_view basename_cstr =
            path_cstr_basename(ti->path.begin);

        const struct string_view basename_sv = path_sv_basename(ti->path);

        check_sv_set_result(basename_cstr, ti->basename, result);
        check_sv_set_result(basename_sv, ti->basename, result);

        const struct string_view filename_with_ext_cstr =
            path_cstr_filename(ti->path.begin);
        const struct string_view filename_with_ext_sv =
            path_sv_filename(ti->path);

        check_sv_set_result(filename_with_ext_cstr,
                            ti->filename_with_ext,
                            result);
        check_sv_set_result(filename_with_ext_sv,
                            ti->filename_with_ext,
                            result);

        const struct string_view extension_cstr =
            path_cstr_extension(ti->path.begin);

        const struct string_view extension_sv = path_sv_extension(ti->path);

        check_sv_set_result(extension_cstr, ti->extension, result);
        check_sv_set_result(extension_sv, ti->extension, result);

        const struct string_view filename_wo_ext_cstr =
            path_cstr_filename_without_extension(ti->path.begin);
        const struct string_view filename_wo_ext_sv =
            path_sv_filename_without_extension(ti->path);

        check_sv_set_result(filename_wo_ext_cstr, ti->filename_wo_ext, result);
        check_sv_set_result(filename_wo_ext_sv, ti->filename_wo_ext, result);

        uint32_t comp_index = 0;
        path_cstr_foreach_component(ti->path.begin, comp, /*skip_root=*/false) {
            if (!index_in_bounds(comp_index, ti->components_count)) {
                printf("Error: Exceeded expected component count\n");
                result = false;
            }

            check_sv_set_result(comp, ti->components[comp_index], result);
            comp_index++;
        }

        comp_index = 0;
        path_sv_foreach_component(ti->path, comp, /*skip_root=*/false) {
            if (!index_in_bounds(comp_index, ti->components_count)) {
                printf("Error: Exceeded expected component count\n");
                result = false;
            }

            check_sv_set_result(comp, ti->components[comp_index], result);
            comp_index++;
        }

        comp_index = ti->non_root_start_index;
        path_cstr_foreach_component(ti->path.begin, comp, /*skip_root=*/true) {
            if (!index_in_bounds(comp_index, ti->components_count)) {
                printf("Error: Exceeded expected component count\n");
                result = false;
            }

            check_sv_set_result(comp, ti->components[comp_index], result);
            comp_index++;
        }

        comp_index = ti->non_root_start_index;
        path_sv_foreach_component(ti->path, comp, /*skip_root=*/true) {
            if (!index_in_bounds(comp_index, ti->components_count)) {
                printf("Error: Exceeded expected component count\n");
                result = false;
            }

            check_sv_set_result(comp, ti->components[comp_index], result);
            comp_index++;
        }
    }

    if (!result) {
        printf("path: Some tests failed!\n");
        return 1;
    }

    printf("path: All tests passed!\n");
    return 0;
}
