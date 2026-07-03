---
name: add-module
description: Add a new library module to this project — scaffolds header, source, test, example, and CMake targets, then walks the implementation checklist. Use when asked to add a module, library, or new subsystem.
---

# Add a Library Module

## Step 1: Scaffold

Run the deterministic scaffolder instead of creating files by hand:

```bash
scripts/new_module.sh <module> [file]
# e.g. scripts/new_module.sh networking socket
```

Names must be `lowercase_snake`. This creates `include/<module>/<file>.h`,
`src/<module>/<file>.c`, `tests/test_<module>.c`, `examples/<module>_demo.c`,
and registers all CMake targets. Do NOT register targets manually afterward —
the script already did it.

## Step 2: Implement

Replace the generated `<file>_greet` placeholder with the real API. Follow the
conventions in AGENTS.md strictly:

- Fallible functions return `ErrorCode` (from `core/error.h`); outputs via pointer params
- Validate all pointer parameters first (`ERR_INVALID_ARG`), check allocations (`ERR_NOMEM`)
- Resources get `init`/`destroy` pairs; clean up in reverse order on error paths
- Doxygen comment on every public declaration; state who frees any allocated memory
- Functions named `<module>_<action>` or `<file>_<action>`

If the module needs more source files, add them to the module's existing
`add_library()` block in the root `CMakeLists.txt`.

If it needs libraries beyond `core`, extend its `target_link_libraries()` line
(e.g. `m` for math, `Threads::Threads` for pthreads — see the `hpc` block for the pattern).

## Step 3: Test

Extend `tests/test_<module>.c` with real assertions — at minimum: NULL/invalid-input
rejection, a success path, and edge cases. CI enforces 80% line coverage, so cover
every public function.

## Step 4: Verify (must pass before you're done)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build
ctest --test-dir build --output-on-failure
clang-format -i $(git ls-files '*.c' '*.h' ':!third_party/*')
```

## Step 5: Update documentation

In `docs/ARCHITECTURE.md`, update all three: the **Module Dependency Graph**,
the **Test Targets** table, and the **Example Targets** table. Add the module to
the Modules list in `README.md` and the Project Map in `AGENTS.md` if it is a
core part of the template.
