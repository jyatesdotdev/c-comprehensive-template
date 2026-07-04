# core public API (`include/core/`)

The vocabulary every module shares. General header rules: `include/AGENTS.md`;
implementation invariants: `src/core/AGENTS.md`.

## Consumer contract

- `error.h` — **the** `ErrorCode` enum. `ERR_OK` is 0, so `if (err) return err;`
  propagates. Match on specific codes (`ERR_NOT_FOUND` vs `ERR_IO` mean
  different recoveries); render with `error_str()`. `LOG_ERROR_CODE(code)`
  logs with file/line.
- `log.h` — use the `LOG_DEBUG/INFO/WARN/ERROR(...)` macros, printf-style
  (format-checked). Default level is INFO; `log_set_level(LOG_LEVEL_DEBUG)`
  in demos/debugging. Each call emits one atomic line — don't build
  multi-line log output from consecutive calls and expect adjacency under
  threads.
- `time.h` — durations come from `time_now_ns/ms` or `Stopwatch`
  (monotonic; epoch is arbitrary, only differences are meaningful). Never
  measure elapsed time with wall-clock APIs.

Nothing in core returns allocated memory to callers; there is no ownership
to manage here.
