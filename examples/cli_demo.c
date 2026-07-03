/**
 * @file cli_demo.c
 * @brief CLI demo with subcommands showing argparse + output formatting.
 *
 * Usage:
 *   cli_demo init --name myproject --template lib
 *   cli_demo run --config app.conf --jobs 4 --verbose
 *   cli_demo status
 *   cli_demo --help
 */
#include "cli/argparse.h"
#include "cli/output.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Subcommand: init ───────────────────────────────────────────────────── */

static int cmd_init(int argc, char **argv) {
    static const CliOption opts[] = {
        {"name", 'n', CLI_REQUIRED_ARG, "Project name", "PROJECT_NAME", NULL},
        {"template", 't', CLI_REQUIRED_ARG, "Template type", NULL, "app"},
        {"help", 'h', CLI_NO_ARG, "Show help", NULL, NULL},
    };
    enum { NUM_OPTS = sizeof(opts) / sizeof(opts[0]) };

    CliContext ctx;
    if (cli_parse(argc, argv, opts, NUM_OPTS, &ctx) != ERR_OK) return 1;

    if (cli_flag(&ctx, "help")) {
        cli_print_help("cli_demo init", "Initialize a new project", opts, NUM_OPTS, NULL, 0);
        return 0;
    }

    const char *name = cli_resolve(&ctx, "name");
    const char *tmpl = cli_resolve(&ctx, "template");

    if (!name) {
        cli_color_fprintf(stderr, CLR_RED, "error: --name is required\n");
        return 1;
    }

    cli_color_fprintf(stdout, CLR_GREEN, "✓ Initialized project '%s' with template '%s'\n", name,
                      tmpl);
    return 0;
}

/* ── Subcommand: run ────────────────────────────────────────────────────── */

static int cmd_run(int argc, char **argv) {
    static const CliOption opts[] = {
        {"config", 'c', CLI_REQUIRED_ARG, "Config file path", "APP_CONFIG", NULL},
        {"jobs", 'j', CLI_REQUIRED_ARG, "Parallel jobs", "APP_JOBS", "2"},
        {"verbose", 'v', CLI_NO_ARG, "Verbose output", "APP_VERBOSE", NULL},
        {"help", 'h', CLI_NO_ARG, "Show help", NULL, NULL},
    };
    enum { NUM_OPTS = sizeof(opts) / sizeof(opts[0]) };

    CliContext ctx;
    if (cli_parse(argc, argv, opts, NUM_OPTS, &ctx) != ERR_OK) return 1;

    if (cli_flag(&ctx, "help")) {
        cli_print_help("cli_demo run", "Run the build pipeline", opts, NUM_OPTS, NULL, 0);
        return 0;
    }

    /* Load config file if provided. */
    const char *cfg = cli_resolve(&ctx, "config");
    if (cfg) {
        if (cli_load_config(cfg, &ctx) == ERR_OK) {
            if (cli_flag(&ctx, "verbose"))
                cli_color_fprintf(stderr, CLR_DIM, "Loaded config: %s\n", cfg);
        } else {
            cli_color_fprintf(stderr, CLR_YELLOW, "warning: config '%s' not found\n", cfg);
        }
    }

    int jobs = atoi(cli_resolve(&ctx, "jobs"));
    if (cli_flag(&ctx, "verbose")) printf("Running with %d job(s)\n", jobs);

    /* Demo progress bar. */
    int total = 20;
    for (int i = 0; i <= total; i++) {
        cli_progress(i, total, 30, stderr);
        /* Simulate work — in real code this would be actual processing. */
        for (volatile long k = 0; k < 2000000; k++) {}
    }
    cli_progress_done(stderr);

    cli_color_fprintf(stdout, CLR_GREEN, "✓ Build complete\n");
    return 0;
}

/* ── Subcommand: status ─────────────────────────────────────────────────── */

static int cmd_status(int argc, char **argv) {
    (void)argc;
    (void)argv;

    cli_color_fprintf(stdout, CLR_BOLD, "Project Status\n\n");

    const int   widths[] = {20, 12, 30};
    const char *headers[] = {"Component", "Status", "Details"};
    CliTable    tbl;
    cli_table_init(&tbl, stdout, 3, widths, headers);
    cli_table_header(&tbl);

    const char *r1[] = {"core", "ok", "v1.2.0"};
    const char *r2[] = {"cli", "ok", "argparse + output"};
    const char *r3[] = {"tests", "passing", "42/42 assertions"};
    const char *r4[] = {"docs", "stale", "last updated 2026-03"};
    cli_table_row(&tbl, r1);
    cli_table_row(&tbl, r2);
    cli_table_row(&tbl, r3);
    cli_table_row(&tbl, r4);
    cli_table_separator(&tbl);

    return 0;
}

/* ── Main ───────────────────────────────────────────────────────────────── */

int main(int argc, char **argv) {
    static const CliSubcommand cmds[] = {
        {"init", "Initialize a new project", cmd_init},
        {"run", "Run the build pipeline", cmd_run},
        {"status", "Show project status table", cmd_status},
    };
    return cli_dispatch(argc, argv, cmds, 3, "cli_demo — CLI library demonstration");
}
