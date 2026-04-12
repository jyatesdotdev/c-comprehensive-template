# CLI Development Guide

This guide covers building command-line interfaces in C using the `cli` library
provided by this template. The library wraps POSIX `getopt_long` with layered
value resolution, subcommand dispatch, and terminal output formatting.

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [Argument Parsing with getopt_long](#argument-parsing-with-getopt_long)
- [Subcommand Dispatch](#subcommand-dispatch)
- [Value Resolution: CLI → Env → Config → Default](#value-resolution)
- [Config File Loading](#config-file-loading)
- [Environment Variable Fallback](#environment-variable-fallback)
- [Structured Parsing with argtable3](#structured-parsing-with-argtable3)
- [Output Formatting](#output-formatting)
- [Testing CLI Logic](#testing-cli-logic)
- [Best Practices](#best-practices)

---

## Architecture Overview

```
include/cli/
  argparse.h    — Option definitions, parsing, dispatch, resolution
  output.h      — ANSI colors, tables, progress bars

src/cli/
  argparse.c    — getopt_long wrapper, config loader, resolver
  output.c      — Terminal output helpers

examples/
  cli_demo.c           — Full subcommand demo (getopt_long)
  cli_argtable_demo.c  — argtable3 alternative demo
```

The `cli` library links against `core` (for `ErrorCode`) and is built as a
static library target in CMake.

---

## Argument Parsing with getopt_long

### Defining Options

Each option is declared as a `CliOption` struct:

```c
#include "cli/argparse.h"

static const CliOption opts[] = {
    {"output",  'o', CLI_REQUIRED_ARG, "Output file path",  "APP_OUTPUT", NULL},
    {"verbose", 'v', CLI_NO_ARG,       "Enable verbose",    "APP_VERBOSE", NULL},
    {"jobs",    'j', CLI_REQUIRED_ARG, "Parallel jobs",     "APP_JOBS",    "2"},
    {"help",    'h', CLI_NO_ARG,       "Show help",         NULL,          NULL},
};
```

Fields:
- `long_name` — the `--flag` name (also used as the key for resolution)
- `short_name` — single-char alias (`-v`), or `'\0'` for long-only
- `arg_req` — `CLI_NO_ARG`, `CLI_REQUIRED_ARG`, or `CLI_OPTIONAL_ARG`
- `description` — help text
- `env_var` — environment variable fallback name, or `NULL`
- `default_val` — default value string, or `NULL`

### Parsing

```c
CliContext ctx;
ErrorCode err = cli_parse(argc, argv, opts, NUM_OPTS, &ctx);
if (err != ERR_OK) return 1;
```

`cli_parse` resets `getopt` state (including `optreset` on macOS), builds the
`struct option[]` and short-option string internally, then populates
`ctx.cli_args`. Non-option arguments are available via `ctx.rest_argc` /
`ctx.rest_argv`.

### Retrieving Values

```c
const char *output = cli_resolve(&ctx, "output");  // layered resolution
bool verbose = cli_flag(&ctx, "verbose");           // boolean check
```

`cli_resolve` returns the first non-NULL value from the priority chain (see
[Value Resolution](#value-resolution)). `cli_flag` returns `true` if the
resolved value is empty (flag present), `"1"`, `"true"`, or `"yes"`.

---

## Subcommand Dispatch

Define a table of subcommands and call `cli_dispatch`:

```c
static int cmd_init(int argc, char **argv) { /* ... */ }
static int cmd_run(int argc, char **argv)  { /* ... */ }

static const CliSubcommand cmds[] = {
    {"init", "Initialize a project", cmd_init},
    {"run",  "Run the pipeline",     cmd_run},
};

int main(int argc, char **argv) {
    return cli_dispatch(argc, argv, cmds, 2, "mytool — description");
}
```

`cli_dispatch` checks `argv[1]` against the command table. If matched, it calls
the handler with `argc-1, argv+1` so the handler sees its own options starting
at `argv[1]`. If no match or `--help` is passed, it prints help and returns.

Each subcommand handler defines its own `CliOption[]` and calls `cli_parse`
independently, giving each command its own flags.

---

## Value Resolution

`cli_resolve` implements a four-layer priority chain:

| Priority | Source | Example |
|----------|--------|---------|
| 1 (highest) | Command-line argument | `--jobs 4` |
| 2 | Environment variable | `APP_JOBS=8` |
| 3 | Config file entry | `jobs=6` |
| 4 (lowest) | Default value | `"2"` in CliOption |

```c
// With --jobs 4 on CLI and APP_JOBS=8 in env:
cli_resolve(&ctx, "jobs");  // returns "4" (CLI wins)

// Without --jobs on CLI but APP_JOBS=8 in env:
cli_resolve(&ctx, "jobs");  // returns "8" (env wins)

// With nothing set:
cli_resolve(&ctx, "jobs");  // returns "2" (default)
```

This pattern follows the convention used by tools like Docker, kubectl, and
Terraform where explicit flags override environment which overrides config files.

---

## Config File Loading

Load a `key=value` file into the context:

```c
ErrorCode err = cli_load_config("app.conf", &ctx);
```

File format:
```ini
# Comment lines start with #
output = result.txt
jobs = 4
verbose = true
```

Keys are matched against option `long_name` during resolution. Leading/trailing
whitespace around keys and values is trimmed. Empty lines and `#` comments are
skipped. Values are `strdup`'d internally.

Call `cli_load_config` after `cli_parse` so that CLI arguments retain highest
priority in the resolution chain.

---

## Environment Variable Fallback

Set the `env_var` field in `CliOption` to enable automatic env lookup:

```c
{"output", 'o', CLI_REQUIRED_ARG, "Output path", "APP_OUTPUT", NULL},
```

When `cli_resolve` finds no CLI argument for `"output"`, it checks
`getenv("APP_OUTPUT")` before falling through to config and defaults.

Convention: use `APPNAME_FLAGNAME` in uppercase (e.g., `APP_VERBOSE`,
`APP_JOBS`).

---

## Structured Parsing with argtable3

For projects that want typed argument parsing with built-in validation,
argtable3 is available via FetchContent:

```bash
cmake -DUSE_ARGTABLE3=ON -B build
```

### Defining Typed Arguments

```c
#include "argtable3.h"

struct arg_lit  *help    = arg_lit0("h", "help",    "print help");
struct arg_str  *output  = arg_str0("o", "output",  "<file>", "output file");
struct arg_int  *jobs    = arg_int0("j", "jobs",    "<n>",    "parallel jobs");
struct arg_file *infiles = arg_filen(NULL, NULL,     "<input>", 0, 10, "inputs");
struct arg_end  *end     = arg_end(20);

void *argtable[] = {help, output, jobs, infiles, end};
```

Type constructors: `arg_lit0/1/n`, `arg_str0/1/n`, `arg_int0/1/n`,
`arg_file0/1/n`, `arg_dbl0/1/n`, `arg_rex0/1/n`. The suffix indicates
min occurrences (`0` = optional, `1` = required, `n` = custom range).

### Parsing and Validation

```c
jobs->ival[0] = 1;  // set default before parsing
int nerrors = arg_parse(argc, argv, argtable);

if (nerrors > 0) {
    arg_print_errors(stderr, end, progname);
    return 1;
}

// Access typed values directly
int njobs = jobs->ival[0];
const char *outpath = output->sval[0];
```

### Help Generation

```c
arg_print_syntax(stdout, argtable, "\n");
arg_print_glossary(stdout, argtable, "  %-25s %s\n");
```

### Cleanup

Always free the table when done:

```c
arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
```

### When to Use argtable3 vs getopt_long

| Criteria | getopt_long | argtable3 |
|----------|-------------|-----------|
| Dependencies | None (POSIX) | FetchContent |
| Typed values | Manual `atoi`/`strtol` | Built-in int/dbl/str/file |
| Validation | Manual | Min/max occurrences, regex |
| Help generation | Manual or `cli_print_help` | `arg_print_glossary` |
| Env/config merging | Via `cli_resolve` | Not built-in |
| Subcommands | Via `cli_dispatch` | Manual |

Use getopt_long (with the `cli` library) when you need layered resolution,
subcommands, or zero dependencies. Use argtable3 when you want strict typed
validation and auto-generated help with minimal code.

---

## Output Formatting

### ANSI Colors

```c
#include "cli/output.h"

cli_color_fprintf(stdout, CLR_GREEN, "✓ Success: %s\n", msg);
cli_color_fprintf(stderr, CLR_RED,   "error: %s\n", err);
cli_color_fprintf(stdout, CLR_DIM,   "hint: try --help\n");
```

Colors are automatically disabled when:
- Output is not a TTY (piped or redirected)
- The `NO_COLOR` environment variable is set (see https://no-color.org)

Available macros: `CLR_RESET`, `CLR_BOLD`, `CLR_DIM`, `CLR_RED`, `CLR_GREEN`,
`CLR_YELLOW`, `CLR_BLUE`, `CLR_MAGENTA`, `CLR_CYAN`.

### Table Formatting

```c
const int widths[] = {20, 12, 30};
const char *headers[] = {"Name", "Status", "Details"};

CliTable tbl;
cli_table_init(&tbl, stdout, 3, widths, headers);
cli_table_header(&tbl);

const char *row[] = {"core", "ok", "v1.0.0"};
cli_table_row(&tbl, row);
cli_table_separator(&tbl);
```

Output:
```
Name                 | Status       | Details
---------------------+--------------+------------------------------
core                 | ok           | v1.0.0
---------------------+--------------+------------------------------
```

Maximum 16 columns (`CLI_TABLE_MAX_COLS`). Header labels are truncated to 63
characters.

### Progress Bar

```c
for (int i = 0; i <= total; i++) {
    cli_progress(i, total, 30, stderr);
    // ... do work ...
}
cli_progress_done(stderr);
```

Output (on a single line, updated with `\r`):
```
[==================>           ]  60%
```

Use `stderr` for progress so `stdout` data remains pipeable.

---

## Testing CLI Logic

Tests use the Unity framework. The key technique is constructing synthetic
`argv` arrays and calling `cli_parse` directly:

```c
void test_parse_long_option(void) {
    char *argv[] = {"prog", "--output", "file.txt", NULL};
    CliContext ctx;
    ErrorCode err = cli_parse(3, argv, opts, NUM_OPTS, &ctx);
    TEST_ASSERT_EQUAL(ERR_OK, err);
    TEST_ASSERT_EQUAL_STRING("file.txt", cli_resolve(&ctx, "output"));
}

void test_flag_detection(void) {
    char *argv[] = {"prog", "--verbose", NULL};
    CliContext ctx;
    cli_parse(2, argv, opts, NUM_OPTS, &ctx);
    TEST_ASSERT_TRUE(cli_flag(&ctx, "verbose"));
}
```

### Testing Resolution Priority

```c
void test_env_fallback(void) {
    setenv("APP_OUTPUT", "from_env.txt", 1);
    char *argv[] = {"prog", NULL};
    CliContext ctx;
    cli_parse(1, argv, opts, NUM_OPTS, &ctx);
    TEST_ASSERT_EQUAL_STRING("from_env.txt", cli_resolve(&ctx, "output"));
    unsetenv("APP_OUTPUT");
}
```

### Testing Output

Capture output to a `tmpfile()` and verify contents:

```c
void test_progress_bar(void) {
    FILE *f = tmpfile();
    cli_progress(50, 100, 20, f);
    fflush(f);
    rewind(f);
    char buf[256];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    TEST_ASSERT_NOT_NULL(strstr(buf, "50%"));
    fclose(f);
}
```

### Running Tests

```bash
cmake -B build -DBUILD_TESTING=ON
cmake --build build
cd build && ctest --output-on-failure
```

---

## Best Practices

1. **Always provide `--help`** — include a `help` option in every command and
   subcommand. Call `cli_print_help` when it's set.

2. **Exit codes** — return 0 on success, 1 on user error (bad args), 2 on
   system error. Be consistent across subcommands.

3. **Stderr for diagnostics** — errors, warnings, progress bars, and verbose
   output go to `stderr`. Only primary output goes to `stdout` so it can be
   piped.

4. **Respect `NO_COLOR`** — the output library handles this automatically.
   Never hardcode ANSI escapes; use `cli_color_fprintf`.

5. **Config file path convention** — accept `--config` on CLI, fall back to
   `$APP_CONFIG` env var, then try a default path like `~/.config/app/config`.

6. **Validate early** — check required arguments immediately after parsing.
   Print specific error messages (not just "invalid arguments").

7. **Document env vars** — the help generator shows `[env: VAR_NAME]` next to
   options that have env fallbacks. Keep env var names consistent with a common
   prefix.

8. **Subcommand design** — keep each subcommand in its own function with its
   own option definitions. This keeps the option namespace clean and makes help
   output focused.

9. **Test parsing, not main()** — test `cli_parse` + `cli_resolve` with
   synthetic argv arrays. This avoids process spawning and makes tests fast and
   deterministic.

10. **Cross-platform** — the library handles macOS `optreset` automatically.
    Avoid platform-specific terminal APIs; the ANSI codes used here work on
    macOS, Linux, and modern Windows Terminal.

---

## See Also

- [README](../README.md) — Quick start and module overview
- [TUTORIAL](TUTORIAL.md) — New developer walkthrough
- [ARCHITECTURE](ARCHITECTURE.md) — Project structure and the `cli` library target
- [EXTENDING](EXTENDING.md) — Adding new modules and examples
- [BEST_PRACTICES](BEST_PRACTICES.md) — Error handling patterns used by the CLI library
- [THIRD_PARTY](THIRD_PARTY.md) — argtable3 FetchContent setup
