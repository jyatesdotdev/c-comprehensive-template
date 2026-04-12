/**
 * @file argparse.h
 * @brief CLI argument parsing: getopt_long wrapper, subcommand dispatch,
 *        env var fallback, config file merging, help generation.
 */
#ifndef CLI_ARGPARSE_H
#define CLI_ARGPARSE_H

#include "core/error.h"
#include <stdbool.h>
#include <stddef.h>

/** Maximum options and config entries supported. */
#define CLI_MAX_OPTS   64
#define CLI_MAX_CONFIG 128

/* ── Option Definition ──────────────────────────────────────────────────── */

/** Argument requirement (mirrors getopt values). */
enum CliArgReq {
    CLI_NO_ARG       = 0,
    CLI_REQUIRED_ARG = 1,
    CLI_OPTIONAL_ARG = 2,
};

/** @brief Defines a single CLI option with layered resolution. */
typedef struct {
    const char *long_name;    /**< Long flag name (e.g. "verbose"). */
    char        short_name;   /**< Single-char alias, or '\0'. */
    int         arg_req;      /**< CLI_NO_ARG, CLI_REQUIRED_ARG, CLI_OPTIONAL_ARG. */
    const char *description;  /**< Help text for this option. */
    const char *env_var;      /**< Env var fallback name, or NULL. */
    const char *default_val;  /**< Default value, or NULL. */
} CliOption;

/* ── Subcommand Definition ──────────────────────────────────────────────── */

/** @brief Handler signature: receives argc/argv after the subcommand name. */
typedef int (*CliSubcommandFn)(int argc, char **argv);

/** @brief Defines a subcommand with name, description, and handler. */
typedef struct {
    const char     *name;
    const char     *description;
    CliSubcommandFn handler;
} CliSubcommand;

/* ── Parsed Context ─────────────────────────────────────────────────────── */

/** @brief Stores a resolved key-value pair from any source. */
typedef struct {
    const char *key;
    const char *value;   /**< NULL for boolean flags. */
} CliEntry;

/** @brief Parsing context holding results from all resolution layers. */
typedef struct {
    CliEntry cli_args[CLI_MAX_OPTS];     /**< Values from command line. */
    int      cli_count;
    CliEntry config_vals[CLI_MAX_CONFIG]; /**< Values from config file. */
    int      config_count;
    const CliOption *options;             /**< Option definitions (borrowed). */
    int              option_count;
    const char      *subcommand;         /**< Matched subcommand, or NULL. */
    int              rest_argc;          /**< argc after subcommand/options. */
    char           **rest_argv;          /**< argv after subcommand/options. */
} CliContext;

/* ── Core API ───────────────────────────────────────────────────────────── */

/**
 * @brief Parse argv using getopt_long against the given option definitions.
 *
 * Populates ctx->cli_args. Non-option arguments go to rest_argc/rest_argv.
 * @param argc        Argument count.
 * @param argv        Argument vector.
 * @param options     Array of option definitions.
 * @param num_options Number of entries in options.
 * @param ctx         Parsing context to populate.
 * @return ERR_OK on success, or an error code.
 */
ErrorCode cli_parse(int argc, char **argv,
                    const CliOption *options, int num_options,
                    CliContext *ctx);

/**
 * @brief Dispatch to a subcommand handler.
 *
 * Checks argv[1] against the table.
 * @param argc             Argument count.
 * @param argv             Argument vector.
 * @param cmds             Array of subcommand definitions.
 * @param num_cmds         Number of entries in cmds.
 * @param prog_description Program description for help output.
 * @return The handler's return value, or -1 if no match (prints help hint).
 */
int cli_dispatch(int argc, char **argv,
                 const CliSubcommand *cmds, int num_cmds,
                 const char *prog_description);

/**
 * @brief Load a key=value config file into ctx->config_vals.
 *
 * Lines starting with '#' or empty lines are skipped.
 * Keys are matched against option long_name for resolution.
 * @param path Config file path.
 * @param ctx  Parsing context to populate.
 * @return ERR_OK on success, ERR_IO on file error.
 */
ErrorCode cli_load_config(const char *path, CliContext *ctx);

/**
 * @brief Resolve an option's value with priority: CLI > env var > config > default.
 * @param ctx         Parsing context with resolved values.
 * @param option_name Long name of the option to resolve.
 * @return Resolved value string, or NULL if not set anywhere.
 */
const char *cli_resolve(const CliContext *ctx, const char *option_name);

/**
 * @brief Check if a boolean flag is set (present on CLI or truthy in env/config).
 * @param ctx         Parsing context.
 * @param option_name Long name of the flag to check.
 * @return true if the flag is set, false otherwise.
 */
bool cli_flag(const CliContext *ctx, const char *option_name);

/* ── Help Generation ────────────────────────────────────────────────────── */

/**
 * @brief Print formatted help for options and subcommands to stdout.
 * @param prog_name   Program name for usage line.
 * @param description Program description.
 * @param options     Array of option definitions.
 * @param num_options Number of entries in options.
 * @param cmds        Array of subcommand definitions, or NULL.
 * @param num_cmds    Number of entries in cmds.
 */
void cli_print_help(const char *prog_name, const char *description,
                    const CliOption *options, int num_options,
                    const CliSubcommand *cmds, int num_cmds);

/**
 * @brief Print just the usage line.
 * @param prog_name       Program name.
 * @param has_subcommands Whether to show subcommand placeholder.
 */
void cli_print_usage(const char *prog_name, bool has_subcommands);

#endif /* CLI_ARGPARSE_H */
