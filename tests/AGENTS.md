# Tests (`tests/`)

One `test_<module>.c` per module, registered in `tests/CMakeLists.txt`, run
by CTest. CI enforces **80% line coverage** — new library code without tests
fails the gate.

## The CHECK convention (non-negotiable)

```c
#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)
```

**Never use `assert()` in tests.** Release builds define `NDEBUG`, which
deletes assert bodies entirely — a function call inside `assert` silently
never runs, and a loop waiting on its side effect hangs forever (this
actually hung CI once). CHECK always executes. Function calls inside
`CHECK(...)` are therefore fine.

## What a good module test covers

1. **Invalid arguments first**: NULL pointers, zero sizes, out-of-range —
   every public function, asserting the exact ErrorCode.
2. **The happy path with verified values** — known answers (hash vectors,
   2x3·3x2 matmul by hand), not just "returned ERR_OK".
3. **Boundaries**: empty, full, exactly-at-capacity, wraparound, growth
   thresholds, odd sizes that exercise tail/edge code paths.
4. **Lifecycle abuse**: double-destroy, destroy-then-use where defined,
   NULL destroy — all documented as safe, so test that they are.
5. **The failure paths you can reach** (file-not-found, dimension
   mismatch). Unreachable OOM paths are exempt.

For numerical code, verify against independently computed values
(`CHECK_NEAR` with an explicit tolerance) or a stronger property — the ml
module checks backprop against finite-difference gradients.

## Environmental constraints

Tests run under CTest with a 120s timeout, ASan/UBSan locally, and Valgrind
in CI. Therefore: no external network (loopback and Unix sockets are fine),
no leaks (free everything, close every fd), deterministic seeds for anything
random (`rng_seed` with constants), and temp files created in the working
directory or via `tmpfile()` and removed before exit.

## Registering a test

```cmake
add_executable(test_foo test_foo.c)
target_link_libraries(test_foo PRIVATE core foo)   # + m, hpc, ... as needed
add_test(NAME test_foo COMMAND test_foo)
```

Unity/cmocka-based tests exist (`test_memory_unity.c`, `test_cli.c`) to
demonstrate framework integration — guard them with `if(TARGET ...)` as
`tests/CMakeLists.txt` shows. New tests default to the minimal CHECK style.

`tests/.clang-tidy` relaxes `concurrency-mt-unsafe` (calling `exit()` in a
test binary is fine); everything else from the root config still applies.
