/**
 * @file argparse.c
 * @brief CLI argument parsing, config file loading, and help generation.
 */
#include "cli/argparse.h"

#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Internal helpers ───────────────────────────────────────────────────── */

/**
 * @brief Append a key/value pair to the CLI arguments in a context.
 * @param ctx  CLI context to modify.
 * @param key  Option name.
 * @param value  Option value (may be NULL for flags).
 */
static void ctx_add_cli(CliContext *ctx, const char *key, const char *value) {
    if (ctx->cli_count >= CLI_MAX_OPTS) return;
    ctx->cli_args[ctx->cli_count++] = (CliEntry){.key = key, .value = value};
}

/**
 * @brief Search entries array for a key, returning the last match (highest priority).
 * @param entries  Array of CliEntry to search.
 * @param count    Number of entries.
 * @param key      Key to find.
 * @return Value string, or NULL if not found.
 */
static const char *find_entry(const CliEntry *entries, int count, const char *key) {
    for (int i = count - 1; i >= 0; i--) {
        if (strcmp(entries[i].key, key) == 0) return entries[i].value ? entries[i].value : "";
    }
    return NULL;
}

/**
 * @brief Find an option definition by its long name.
 * @param opts   Array of option definitions.
 * @param count  Number of options.
 * @param name   Long option name to match.
 * @return Pointer to matching CliOption, or NULL.
 */
static const CliOption *find_option(const CliOption *opts, int count, const char *name) {
    for (int i = 0; i < count; i++) {
        if (strcmp(opts[i].long_name, name) == 0) return &opts[i];
    }
    return NULL;
}

/** @brief Strip leading and trailing whitespace in-place. */
static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

/* ── cli_parse ──────────────────────────────────────────────────────────── */

ErrorCode cli_parse(int argc, char **argv, const CliOption *options, int num_options,
                    CliContext *ctx) {
    if (!argv || !options || !ctx) return ERR_INVALID_ARG;

    memset(ctx, 0, sizeof(*ctx));
    ctx->options = options;
    ctx->option_count = num_options;

    /* Build getopt_long structures. */
    struct option long_opts[CLI_MAX_OPTS + 1];
    char          short_str[CLI_MAX_OPTS * 3 + 1];
    int           si = 0;

    for (int i = 0; i < num_options && i < CLI_MAX_OPTS; i++) {
        long_opts[i] = (struct option){
            .name = options[i].long_name,
            .has_arg = options[i].arg_req,
            .flag = NULL,
            .val = options[i].short_name ? options[i].short_name : (256 + i),
        };
        if (options[i].short_name) {
            short_str[si++] = options[i].short_name;
            if (options[i].arg_req == CLI_REQUIRED_ARG) short_str[si++] = ':';
            else if (options[i].arg_req == CLI_OPTIONAL_ARG) {
                short_str[si++] = ':';
                short_str[si++] = ':';
            }
        }
    }
    long_opts[num_options] = (struct option){0};
    short_str[si] = '\0';

    optind = 1; /* Reset getopt state. */
#ifdef __APPLE__
    optreset = 1;
#endif
    int c;
    /* NOLINTNEXTLINE(concurrency-mt-unsafe) — getopt runs once at startup, before threads exist */
    while ((c = getopt_long(argc, argv, short_str, long_opts, NULL)) != -1) {
        if (c == '?') continue; /* Unknown option, getopt prints error. */
        /* Find which option matched. */
        for (int i = 0; i < num_options; i++) {
            int val = options[i].short_name ? options[i].short_name : (256 + i);
            if (c == val) {
                ctx_add_cli(ctx, options[i].long_name, optarg);
                break;
            }
        }
    }

    ctx->rest_argc = argc - optind;
    ctx->rest_argv = argv + optind;
    return ERR_OK;
}

/* ── cli_dispatch ───────────────────────────────────────────────────────── */

int cli_dispatch(int argc, char **argv, const CliSubcommand *cmds, int num_cmds,
                 const char *prog_description) {
    if (argc < 2 || !argv[1]) {
        cli_print_help(argv[0], prog_description, NULL, 0, cmds, num_cmds);
        return -1;
    }

    /* Check for --help before subcommand. */
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        cli_print_help(argv[0], prog_description, NULL, 0, cmds, num_cmds);
        return 0;
    }

    for (int i = 0; i < num_cmds; i++) {
        if (strcmp(argv[1], cmds[i].name) == 0) { return cmds[i].handler(argc - 1, argv + 1); }
    }

    fprintf(stderr, "Unknown command: %s\n", argv[1]);
    cli_print_help(argv[0], prog_description, NULL, 0, cmds, num_cmds);
    return -1;
}

/* ── cli_load_config ────────────────────────────────────────────────────── */

ErrorCode cli_load_config(const char *path, CliContext *ctx) {
    if (!path || !ctx) return ERR_INVALID_ARG;

    FILE *f = fopen(path, "r");
    if (!f) return ERR_NOT_FOUND;

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        char *s = trim(line);
        if (*s == '\0' || *s == '#') continue;

        char *eq = strchr(s, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = trim(s);
        char *val = trim(eq + 1);

        if (ctx->config_count < CLI_MAX_CONFIG) {
            /* Duplicate strings since line buffer is reused. */
            ctx->config_vals[ctx->config_count++] = (CliEntry){
                .key = strdup(key),
                .value = strdup(val),
            };
        }
    }

    if (fclose(f) != 0) return ERR_IO;
    return ERR_OK;
}

/* ── cli_resolve ────────────────────────────────────────────────────────── */

const char *cli_resolve(const CliContext *ctx, const char *option_name) {
    if (!ctx || !option_name) return NULL;

    /* 1. CLI args (highest priority). */
    const char *v = find_entry(ctx->cli_args, ctx->cli_count, option_name);
    if (v) return v;

    /* 2. Environment variable. */
    const CliOption *opt = find_option(ctx->options, ctx->option_count, option_name);
    if (opt && opt->env_var) {
        /* NOLINTNEXTLINE(concurrency-mt-unsafe) — read-only env access; unsafe only alongside setenv from another thread */
        v = getenv(opt->env_var);
        if (v) return v;
    }

    /* 3. Config file. */
    v = find_entry(ctx->config_vals, ctx->config_count, option_name);
    if (v) return v;

    /* 4. Default. */
    if (opt && opt->default_val) return opt->default_val;

    return NULL;
}

/* ── cli_flag ───────────────────────────────────────────────────────────── */

bool cli_flag(const CliContext *ctx, const char *option_name) {
    const char *v = cli_resolve(ctx, option_name);
    if (!v) return false;
    /* Present with no value (flag), or truthy string. */
    return *v == '\0' || strcmp(v, "1") == 0 || strcmp(v, "true") == 0 || strcmp(v, "yes") == 0;
}

/* ── cli_free ───────────────────────────────────────────────────────────── */

void cli_free(CliContext *ctx) {
    if (!ctx) return;
    for (int i = 0; i < ctx->config_count; i++) {
        free((void *)ctx->config_vals[i].key);
        free((void *)ctx->config_vals[i].value);
    }
    ctx->config_count = 0;
}

/* ── Help Generation ────────────────────────────────────────────────────── */

void cli_print_usage(const char *prog_name, bool has_subcommands) {
    fprintf(stdout, "Usage: %s %s[options]\n", prog_name, has_subcommands ? "<command> " : "");
}

void cli_print_help(const char *prog_name, const char *description, const CliOption *options,
                    int num_options, const CliSubcommand *cmds, int num_cmds) {
    if (description) fprintf(stdout, "%s\n\n", description);
    cli_print_usage(prog_name, num_cmds > 0);

    if (num_cmds > 0) {
        fprintf(stdout, "\nCommands:\n");
        for (int i = 0; i < num_cmds; i++) {
            fprintf(stdout, "  %-16s %s\n", cmds[i].name,
                    cmds[i].description ? cmds[i].description : "");
        }
    }

    if (num_options > 0) {
        fprintf(stdout, "\nOptions:\n");
        for (int i = 0; i < num_options; i++) {
            char flag_buf[48];
            if (options[i].short_name)
                snprintf(flag_buf, sizeof(flag_buf), "-%c, --%s", options[i].short_name,
                         options[i].long_name);
            else snprintf(flag_buf, sizeof(flag_buf), "    --%s", options[i].long_name);

            fprintf(stdout, "  %-24s %s", flag_buf,
                    options[i].description ? options[i].description : "");
            if (options[i].env_var) fprintf(stdout, " [env: %s]", options[i].env_var);
            if (options[i].default_val) fprintf(stdout, " (default: %s)", options[i].default_val);
            fputc('\n', stdout);
        }
    }
}
