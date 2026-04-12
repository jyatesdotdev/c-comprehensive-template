/**
 * @file test_cli.c
 * @brief Unity tests for cli/argparse and cli/output modules.
 */
#include "unity.h"
#include "cli/argparse.h"
#include "cli/output.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void setUp(void) {}
void tearDown(void) {}

/* ── Helper: read tmpfile contents into static buffer ───────────────────── */

static char captured[4096];

static const char *capture(FILE *f) {
    fflush(f);
    rewind(f);
    size_t n = fread(captured, 1, sizeof(captured) - 1, f);
    captured[n] = '\0';
    return captured;
}

/* ── Shared option definitions ──────────────────────────────────────────── */

static const CliOption test_opts[] = {
    {"verbose", 'v', CLI_NO_ARG,       "Enable verbose",  "APP_VERBOSE", NULL},
    {"output",  'o', CLI_REQUIRED_ARG,  "Output file",     "APP_OUTPUT",  "out.txt"},
    {"count",   'c', CLI_REQUIRED_ARG,  "Count",           NULL,          "10"},
};
#define NUM_OPTS (int)(sizeof(test_opts) / sizeof(test_opts[0]))

/* ── argparse tests ─────────────────────────────────────────────────────── */

void test_parse_long_option(void) {
    char *argv[] = {"prog", "--output", "foo.txt", NULL};
    CliContext ctx;
    TEST_ASSERT_EQUAL(ERR_OK, cli_parse(3, argv, test_opts, NUM_OPTS, &ctx));
    TEST_ASSERT_EQUAL_STRING("foo.txt", cli_resolve(&ctx, "output"));
}

void test_parse_short_option(void) {
    char *argv[] = {"prog", "-o", "bar.txt", NULL};
    CliContext ctx;
    TEST_ASSERT_EQUAL(ERR_OK, cli_parse(3, argv, test_opts, NUM_OPTS, &ctx));
    TEST_ASSERT_EQUAL_STRING("bar.txt", cli_resolve(&ctx, "output"));
}

void test_parse_flag(void) {
    char *argv[] = {"prog", "--verbose", NULL};
    CliContext ctx;
    TEST_ASSERT_EQUAL(ERR_OK, cli_parse(2, argv, test_opts, NUM_OPTS, &ctx));
    TEST_ASSERT_TRUE(cli_flag(&ctx, "verbose"));
}

void test_parse_null_returns_error(void) {
    TEST_ASSERT_EQUAL(ERR_INVALID_ARG, cli_parse(0, NULL, test_opts, NUM_OPTS, NULL));
}

void test_parse_rest_args(void) {
    char *argv[] = {"prog", "--verbose", "file1", "file2", NULL};
    CliContext ctx;
    cli_parse(4, argv, test_opts, NUM_OPTS, &ctx);
    TEST_ASSERT_EQUAL(2, ctx.rest_argc);
    TEST_ASSERT_EQUAL_STRING("file1", ctx.rest_argv[0]);
}

/* ── resolve priority tests ─────────────────────────────────────────────── */

void test_resolve_default(void) {
    char *argv[] = {"prog", NULL};
    CliContext ctx;
    cli_parse(1, argv, test_opts, NUM_OPTS, &ctx);
    TEST_ASSERT_EQUAL_STRING("out.txt", cli_resolve(&ctx, "output"));
}

void test_resolve_env_over_default(void) {
    setenv("APP_OUTPUT", "env.txt", 1);
    char *argv[] = {"prog", NULL};
    CliContext ctx;
    cli_parse(1, argv, test_opts, NUM_OPTS, &ctx);
    TEST_ASSERT_EQUAL_STRING("env.txt", cli_resolve(&ctx, "output"));
    unsetenv("APP_OUTPUT");
}

void test_resolve_cli_over_env(void) {
    setenv("APP_OUTPUT", "env.txt", 1);
    char *argv[] = {"prog", "-o", "cli.txt", NULL};
    CliContext ctx;
    cli_parse(3, argv, test_opts, NUM_OPTS, &ctx);
    TEST_ASSERT_EQUAL_STRING("cli.txt", cli_resolve(&ctx, "output"));
    unsetenv("APP_OUTPUT");
}

void test_resolve_unknown_returns_null(void) {
    char *argv[] = {"prog", NULL};
    CliContext ctx;
    cli_parse(1, argv, test_opts, NUM_OPTS, &ctx);
    TEST_ASSERT_NULL(cli_resolve(&ctx, "nonexistent"));
}

/* ── cli_flag tests ─────────────────────────────────────────────────────── */

void test_flag_absent_is_false(void) {
    char *argv[] = {"prog", NULL};
    CliContext ctx;
    cli_parse(1, argv, test_opts, NUM_OPTS, &ctx);
    TEST_ASSERT_FALSE(cli_flag(&ctx, "verbose"));
}

void test_flag_env_truthy(void) {
    setenv("APP_VERBOSE", "true", 1);
    char *argv[] = {"prog", NULL};
    CliContext ctx;
    cli_parse(1, argv, test_opts, NUM_OPTS, &ctx);
    TEST_ASSERT_TRUE(cli_flag(&ctx, "verbose"));
    unsetenv("APP_VERBOSE");
}

/* ── config file tests ──────────────────────────────────────────────────── */

void test_load_config(void) {
    FILE *f = tmpfile();
    TEST_ASSERT_NOT_NULL(f);
    fprintf(f, "# comment\n\noutput = config.txt\ncount = 42\n");
    fflush(f);

    /* Write to a real temp path since cli_load_config opens by path. */
    char path[] = "/tmp/test_cli_cfg_XXXXXX";
    int fd = mkstemp(path);
    TEST_ASSERT_TRUE(fd >= 0);
    rewind(f);
    char buf[256];
    size_t n = fread(buf, 1, sizeof(buf), f);
    write(fd, buf, n);
    close(fd);
    fclose(f);

    char *argv[] = {"prog", NULL};
    CliContext ctx;
    cli_parse(1, argv, test_opts, NUM_OPTS, &ctx);
    TEST_ASSERT_EQUAL(ERR_OK, cli_load_config(path, &ctx));
    /* Config is lower priority than CLI but higher than default. */
    TEST_ASSERT_EQUAL_STRING("config.txt", cli_resolve(&ctx, "output"));

    cli_free(&ctx);
    remove(path);
}

void test_load_config_missing_file(void) {
    CliContext ctx = {0};
    ctx.options = test_opts;
    ctx.option_count = NUM_OPTS;
    TEST_ASSERT_EQUAL(ERR_NOT_FOUND, cli_load_config("/tmp/no_such_file_xyz", &ctx));
}

/* ── output: table tests ────────────────────────────────────────────────── */

void test_table_output(void) {
    FILE *f = tmpfile();
    TEST_ASSERT_NOT_NULL(f);

    const int widths[] = {10, 5};
    const char *headers[] = {"Name", "Val"};
    CliTable t;
    cli_table_init(&t, f, 2, widths, headers);
    cli_table_header(&t);

    const char *row[] = {"foo", "42"};
    cli_table_row(&t, row);

    const char *out = capture(f);
    TEST_ASSERT_NOT_NULL(strstr(out, "Name"));
    TEST_ASSERT_NOT_NULL(strstr(out, "foo"));
    TEST_ASSERT_NOT_NULL(strstr(out, "42"));
    fclose(f);
}

/* ── output: progress bar test ──────────────────────────────────────────── */

void test_progress_output(void) {
    FILE *f = tmpfile();
    TEST_ASSERT_NOT_NULL(f);
    cli_progress(50, 100, 20, f);
    const char *out = capture(f);
    TEST_ASSERT_NOT_NULL(strstr(out, "50%"));
    fclose(f);
}

/* ── Runner ─────────────────────────────────────────────────────────────── */

int main(void) {
    UNITY_BEGIN();
    /* argparse */
    RUN_TEST(test_parse_long_option);
    RUN_TEST(test_parse_short_option);
    RUN_TEST(test_parse_flag);
    RUN_TEST(test_parse_null_returns_error);
    RUN_TEST(test_parse_rest_args);
    /* resolve priority */
    RUN_TEST(test_resolve_default);
    RUN_TEST(test_resolve_env_over_default);
    RUN_TEST(test_resolve_cli_over_env);
    RUN_TEST(test_resolve_unknown_returns_null);
    /* cli_flag */
    RUN_TEST(test_flag_absent_is_false);
    RUN_TEST(test_flag_env_truthy);
    /* config */
    RUN_TEST(test_load_config);
    RUN_TEST(test_load_config_missing_file);
    /* output */
    RUN_TEST(test_table_output);
    RUN_TEST(test_progress_output);
    return UNITY_END();
}
