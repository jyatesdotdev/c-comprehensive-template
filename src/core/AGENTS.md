# core — errors, logging, time (`src/core/`)

The foundation module: every other module links `core`, so nothing here may
depend on any other module. Think twice before adding to core — a utility
belongs here only if it's needed by *most* modules (that bar admitted
logging and monotonic time; it keeps out anything domain-specific).

## Files

- `error.c` — `error_str` table + `LOG_ERROR_CODE` backend. The `ErrorCode`
  enum in `error.h` is the project-wide error vocabulary; add a new code
  only when no existing one fits, and update `error_strings` in the same
  edit (the designated-initializer table makes a missing entry a NULL —
  `error_str` bounds-checks, but don't rely on it).
- `log.c` — leveled logger. Key invariant: **one `fprintf` call per log
  line**, so concurrent threads never interleave partial messages (stdio
  streams lock per call, not per sequence). Preserve this if you touch the
  formatting. Color detection duplicates `cli_colors_enabled` deliberately —
  core cannot depend on cli.
- `time.c` — CLOCK_MONOTONIC wrappers. Always monotonic for durations;
  wall-clock (`CLOCK_REALTIME`) appears only in log timestamps where
  human-readable time is the point. `time_sleep_ms` resumes after EINTR by
  feeding `nanosleep`'s remainder back in — the standard pattern.

## C lessons this module encodes

- **Enum-indexed string tables** with designated initializers stay correct
  under reordering: `[ERR_IO] = "I/O error"`.
- **Static mutable globals** (the logger's level/stream) are acceptable for
  process-wide facilities; keep them file-`static`, mutate through setters,
  and document the thread-safety story honestly.
- **ISO C forbids `static FILE *g = stderr;`** (stderr isn't a constant
  expression) — hence the `NULL means stderr` convention in log.c.
