# cli public API (`include/cli/`)

Declarative option parsing and terminal output. General header rules:
`include/AGENTS.md`; getopt state and internals: `src/cli/AGENTS.md`.

## Consumer contract

- `argparse.h` — describe options as a `CliOption` table (long name, short
  char, arg requirement, help text, env var, default), call `cli_parse`
  once at startup, then read everything through `cli_resolve`/`cli_flag`.
  Resolution priority is **CLI > env > config file > default** — rely on
  it, don't reimplement precedence. `cli_load_config` reads `key = value`
  lines (`#` comments); call `cli_free` when a config was loaded (it owns
  the duplicated strings). Positional args land in `rest_argc`/`rest_argv`.
- `output.h` — all color flows through `cli_colors_enabled` (TTY +
  `NO_COLOR` aware); write output so it degrades to plain text.
  `cli_color_fprintf` is format-checked like printf. Tables cap at
  `CLI_TABLE_MAX_COLS`; progress bars write `\r` to their stream —
  finish with `cli_progress_done` before printing anything else.

Parsing is startup-time, single-threaded work — don't call `cli_parse`
concurrently or repeatedly per-request.
