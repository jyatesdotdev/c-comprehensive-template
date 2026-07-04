# cli — argument parsing and terminal output (`src/cli/`)

getopt_long-based option parsing with a layered resolution order, plus ANSI
color/table/progress output. Links `core`.

## The resolution order (the module's core idea)

`cli_resolve` checks sources in priority order: **CLI args > environment
variable > config file > default**. Preserve this order — it's the standard
Unix expectation and `test_cli.c` pins it. Options declare their env var and
default in the `CliOption` table, so adding a source means touching
`cli_resolve` only.

## Invariants and gotchas

- getopt state is global; `cli_parse` resets `optind = 1` (plus `optreset`
  on macOS — keep the `#ifdef __APPLE__`). Parsing is single-threaded
  startup work; the `concurrency-mt-unsafe` NOLINTs document that policy.
- Long-only options get synthetic `val`s of `256 + i` — printable chars are
  reserved for real short names.
- Config parsing duplicates key/value strings (`strdup`, checked) because
  the line buffer is reused; `cli_free` owns releasing them.
- Respect `NO_COLOR` and non-TTY streams: all color goes through
  `cli_colors_enabled`. Never emit escape codes unconditionally.
- `cli_color_fprintf` carries a printf format attribute
  (`CLI_PRINTF_FORMAT`) so callers get format-string checking — any new
  varargs function here must do the same.

## C lessons this module encodes

- Wrapping a crusty POSIX API (getopt) behind a declarative table is the
  pattern: callers describe options as data, the wrapper owns the state
  machine.
- Fixed caps (`CLI_MAX_OPTS`, `CLI_TABLE_MAX_COLS`) with explicit bounds
  checks are an honest choice for CLI-scale data — document the cap in the
  header and enforce it, rather than allocating unboundedly.
