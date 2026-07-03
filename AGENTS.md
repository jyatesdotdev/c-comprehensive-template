# Agent Guide

C17 project template: static library modules under `src/<module>/` + `include/<module>/`,
built with CMake, tested with CTest. This file contains everything you need for routine
changes; the `docs/` guides cover depth.

## Build & Test Loop

Run this after every change. Develop with sanitizers ON.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON   # configure once
cmake --build build                                              # rebuild
ctest --test-dir build --output-on-failure                       # run all tests
```

Format before committing (CI rejects unformatted code):

```bash
clang-format -i $(git ls-files '*.c' '*.h' ':!third_party/*')
```

CI (`.github/workflows/ci.yml`) enforces: clean build, all tests pass, clang-format
compliance, and **line coverage ≥ 80%** — new code without tests will fail the gate.

## Project Map

| Path | Contents |
|------|----------|
| `include/<module>/` | Public headers (Doxygen-documented) |
| `src/<module>/` | Implementations (mirrors `include/`) |
| `tests/` | One `test_<name>.c` per module + `tests/CMakeLists.txt` |
| `examples/` | One `<name>_demo.c` per module + `examples/CMakeLists.txt` |
| `cmake/` | Platform detection, security tooling, test frameworks, third-party deps |
| `scripts/` | `new_module.sh` — scaffolds a complete new module |

Modules: `core` (errors + allocators), `cli`, `systems`, `hpc`, `simulation`,
`rendering_sw` (+ optional `rendering`). Every module depends on `core`.

## Non-Negotiable Conventions

1. **Fallible functions return `ErrorCode`** (`core/error.h`); results go out via
   pointer parameters. `ERR_OK` is 0, so `if (err) return err;` works.
   ```c
   ErrorCode file_read_all(const char *path, unsigned char **out_buf, size_t *out_size);
   ```
2. **Validate pointers first.** Return `ERR_INVALID_ARG` for NULL/invalid inputs before
   doing any work. Check every allocation (`ERR_NOMEM`).
3. **init/destroy pairs.** Resources are acquired with `x_init`/`x_create` and released
   with `x_destroy`. On error paths, release in reverse acquisition order (or use a
   single `goto cleanup:` label). Nullify freed pointers.
4. **Naming:** types `PascalCase` (`Arena`), functions `module_action` (`arena_init`),
   macros `UPPER_SNAKE`. Header guards are `<MODULE>_<FILE>_H` (`MEMORY_ARENA_H`).
5. **Doxygen on every public declaration** (`@file`, `@brief`, `@param`, `@return`).
   If a function allocates, its header comment must say who frees.
6. **Portability:** `size_t` for sizes, `<stdint.h>` for fixed-width. Platform-specific
   code stays in `systems/` or behind `#ifdef`. SIMD code always has a scalar fallback.

Formatting is enforced by `.clang-format` — don't hand-align; run the formatter.

## Adding a New Module

Use the scaffolder — it creates all files and registers all targets:

```bash
scripts/new_module.sh <module> <file>     # e.g. scripts/new_module.sh networking socket
```

It touches these four places (do the same by hand if you customize):

1. `include/<module>/<file>.h` + `src/<module>/<file>.c`
2. Root `CMakeLists.txt` — `add_library(<module> STATIC ...)` block before the
   `# ── Testing Frameworks` section
3. `tests/CMakeLists.txt` — `add_executable` + `target_link_libraries` + `add_test`
4. `examples/CMakeLists.txt` — `add_executable(example_<module> ...)`

Then: implement, add real test assertions, rebuild, and update the module list and
target tables in `docs/ARCHITECTURE.md` (Module Dependency Graph, Test Targets,
Example Targets). Full walkthrough: `docs/EXTENDING.md`.

## When to Read the Long Docs

| Task | Read |
|------|------|
| Adding modules, tests, examples, dependencies | `docs/EXTENDING.md` |
| Understanding build flow / CMake options | `docs/ARCHITECTURE.md` |
| Error handling, memory safety, API design details | `docs/BEST_PRACTICES.md` |
| CLI subcommands / output formatting | `docs/CLI.md` |
| Platform-specific behavior | `docs/CROSS_PLATFORM.md` |
| Sanitizers, static analysis, security gates | `docs/SECURITY_SCANNING.md` |
| Performance work | `docs/OPTIMIZATION.md` |

## Definition of Done

- [ ] Code follows the conventions above and is clang-formatted
- [ ] Every new public function has a Doxygen comment and a test
- [ ] `cmake --build build` clean (warnings are treated seriously; CI uses `-Wall -Wextra -Wpedantic -Wconversion`)
- [ ] `ctest --test-dir build --output-on-failure` passes with sanitizers ON
- [ ] New modules registered in all four places listed above
- [ ] `docs/ARCHITECTURE.md` tables updated if targets were added or removed
