/*
 * This file is part of mpv.
 *
 * mpv is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * mpv is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with mpv.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <inttypes.h>
#include <libmpv/client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Dummy values for the test.
const char *str = "string";
int flag = 1;
int64_t int_ = 20;
double double_ = 1.5;

// Global handle.
mpv_handle *ctx;

static inline void fail(void)
{
    mpv_destroy(ctx);
    exit(1);
}

static inline void check_api_error(int status)
{
    if (status < 0) {
        printf("mpv API error: %s\n", mpv_error_string(status));
        fail();
    }
}

static inline void check_double(const char *property)
{
    double result_double;
    check_api_error(mpv_get_property(ctx, property, MPV_FORMAT_DOUBLE, &result_double));
    if (double_ != result_double) {
        printf("Double: expected '%f' but got '%f'!\n", double_, result_double);
        fail();
    }
}

static inline void check_flag(const char *property)
{
    int result_flag;
    check_api_error(mpv_get_property(ctx, property, MPV_FORMAT_FLAG, &result_flag));
    if (flag != result_flag) {
        printf("Flag: expected '%d' but got '%d'!\n", flag, result_flag);
        fail();
    }
}

static inline void check_int(const char *property)
{
    int64_t result_int;
    check_api_error(mpv_get_property(ctx, property, MPV_FORMAT_INT64, &result_int));
    if (int_ != result_int) {
        printf("Int: expected '%" PRId64 "' but got '%" PRId64 "'!\n", int_, result_int);
        fail();
    }
}

static inline void check_string(const char *property)
{
    char *result_string;
    check_api_error(mpv_get_property(ctx, property, MPV_FORMAT_STRING, &result_string));
    if (strcmp(str, result_string) != 0) {
        printf("Expected '%s' but got '%s'!\n", str, result_string);
        mpv_free(result_string);
        fail();
    }
    mpv_free(result_string);
}

static inline void check_results(const char *properties[], enum mpv_format formats[])
{
    for (int i = 0; properties[i]; i++) {
        switch (formats[i]) {
        case MPV_FORMAT_STRING:
            check_string(properties[i]);
            break;
        case MPV_FORMAT_FLAG:
            check_flag(properties[i]);
            break;
        case MPV_FORMAT_INT64:
            check_int(properties[i]);
            break;
        case MPV_FORMAT_DOUBLE:
            check_double(properties[i]);
            break;
        }
    }
}

static inline void set_options_and_properties(const char *options[], const char *properties[],
                                              enum mpv_format formats[])
{
    for (int i = 0; options[i]; i++) {
        switch (formats[i]) {
        case MPV_FORMAT_STRING:
            check_api_error(mpv_set_option(ctx, options[i], formats[i], &str));
            check_api_error(mpv_set_property(ctx, properties[i], formats[i], &str));
            break;
        case MPV_FORMAT_FLAG:
            check_api_error(mpv_set_option(ctx, options[i], formats[i], &flag));
            check_api_error(mpv_set_property(ctx, properties[i], formats[i], &flag));
            break;
        case MPV_FORMAT_INT64:
            check_api_error(mpv_set_option(ctx, options[i], formats[i], &int_));
            check_api_error(mpv_set_property(ctx, properties[i], formats[i], &int_));
            break;
        case MPV_FORMAT_DOUBLE:
            check_api_error(mpv_set_option(ctx, options[i], formats[i], &double_));
            check_api_error(mpv_set_property(ctx, properties[i], formats[i], &double_));
            break;
        }
    }
}

static void test_file_loading(char *file)
{
    const char *cmd[] = {"loadfile", file, NULL};
    check_api_error(mpv_command(ctx, cmd));
    int loaded = 0;
    int finished = 0;
    time_t end = time(NULL) + 10;
    while (time(NULL) < end) {
        if (finished)
            break;
        mpv_event *event = mpv_wait_event(ctx, 0);
        switch (event->event_id) {
        case MPV_EVENT_FILE_LOADED:
            // make sure it loads before exiting
            loaded = 1;
            break;
        case MPV_EVENT_END_FILE:
            if (loaded)
                finished = 1;
            break;
        }
    }
    if (!finished) {
        printf("Unable to load test file!\n");
        fail();
    }
}

static void test_lavfi_complex(char *file)
{
    const char *cmd[] = {"loadfile", file, NULL};
    check_api_error(mpv_command(ctx, cmd));
    int finished = 0;
    int loaded = 0;
    time_t end = time(NULL) + 10;
    while (time(NULL) < end) {
        if (finished)
            break;
        mpv_event *event = mpv_wait_event(ctx, 0);
        switch (event->event_id) {
        case MPV_EVENT_FILE_LOADED:
            // Add file as external and toggle lavfi-complex on.
            if (!loaded) {
                check_api_error(mpv_set_property_string(ctx, "external-files", file));
                const char *add_cmd[] = {"video-add", file, "auto", NULL};
                check_api_error(mpv_command(ctx, add_cmd));
                check_api_error(mpv_set_property_string(ctx, "lavfi-complex", "[vid1] [vid2] vstack [vo]"));
            }
            loaded = 1;
            break;
        case MPV_EVENT_END_FILE:
            if (loaded)
                finished = 1;
            break;
        }
    }
    if (!finished) {
        printf("Lavfi complex failed!\n");
        fail();
    }
}

static void test_options_and_properties(void)
{
    // Order matters. string -> flag -> int -> double (repeat)
    // One for set_option the other for set_property
    const char *options[] = {
        "screen-name",
        "save-position-on-quit",
        "cursor-autohide",
        "speed",
        NULL
    };

    const char *properties[] = {
        "fs-screen-name",
        "shuffle",
        "sub-pos",
        "window-scale",
        NULL
    };

    // Must match above ordering.
    enum mpv_format formats[] = {
        MPV_FORMAT_STRING,
        MPV_FORMAT_FLAG,
        MPV_FORMAT_INT64,
        MPV_FORMAT_DOUBLE,
    };

    set_options_and_properties(options, properties, formats);

    check_api_error(mpv_initialize(ctx));

    check_results(options, formats);
    check_results(properties, formats);

    // Ensure the format is still MPV_FORMAT_FLAG for these property types.
    mpv_node result_node;
    check_api_error(mpv_get_property(ctx, "idle-active", MPV_FORMAT_NODE, &result_node));
    if (result_node.format != MPV_FORMAT_FLAG) {
        printf("Node: expected mpv format '%d' but got '%d'!\n", MPV_FORMAT_FLAG, result_node.format);
        fail();
    }

    // Always should be true.
    if (result_node.u.flag != 1) {
        printf("Node: expected 1 but got %d'!\n", result_node.u.flag);
        fail();
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
        return 1;

    ctx = mpv_create();
    if (!ctx)
        return 1;

    // Use tct for all video-related stuff.
    check_api_error(mpv_set_property_string(ctx, "vo", "tct"));

    test_options_and_properties();
    test_file_loading(argv[1]);
    test_lavfi_complex(argv[1]);

    mpv_destroy(ctx);
    return 0;
}
