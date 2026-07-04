# testing public API (`include/testing/`)

Header-only performance measurement — no `src/` counterpart, no library
target; just include `testing/perf_test.h`.

## Consumer contract

- `PERF_BENCH(label, iters, { code; })` runs the block `iters` times and
  prints min/avg/max milliseconds. The block is a compound statement —
  braces required. Keep per-iteration work meaningful (microseconds+);
  sub-microsecond bodies mostly measure clock overhead.
- `PERF_START(label)` / `PERF_END(label)` bracket one region ad hoc.
- Results are wall-clock and machine-dependent: use them for *relative*
  comparisons in one run (naive vs blocked matmul), never as CI
  assertions — timing-based test failures are flakiness by construction.
- Benchmark **Release builds**. Debug + sanitizers distorts ratios by
  10-100x (a lesson `example_matmul_bench` demonstrates).

## Maintainer notes

This header predates `core/time.h` and keeps its own `clock_gettime`
calls so it stays a zero-dependency single include — that's deliberate;
don't "fix" it by adding a core dependency. Macro locals are underscore-
prefixed (`_min`, `_t`) to avoid capturing user identifiers; keep that
convention if you extend it, and remember macro arguments referenced more
than once must be side-effect-free (documented in `PERF_BENCH`'s usage).
