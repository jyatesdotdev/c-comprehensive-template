# C Best Practices Guide

This guide documents the coding standards, safety patterns, and modern C practices used throughout this template. All examples reference actual code in the project.

---

## Table of Contents

1. [C Standard Selection](#c-standard-selection)
2. [Error Handling](#error-handling)
3. [Memory Safety](#memory-safety)
4. [Resource Management](#resource-management)
5. [API Design](#api-design)
6. [Compiler Warnings & Sanitizers](#compiler-warnings--sanitizers)
7. [Portability](#portability)
8. [Concurrency Safety](#concurrency-safety)
9. [Performance Considerations](#performance-considerations)

---

## C Standard Selection

This project targets **C17** (`-std=c17`) as the default, with awareness of C11 and C23 features.

### C11 Features Used
- `_Alignof` / `alignof` — alignment queries for custom allocators
- `<stdatomic.h>` — lock-free atomics for thread pool signaling
- `<threads.h>` awareness (we use pthreads for broader platform support)
- Anonymous structs/unions
- `static_assert` — compile-time invariant checks

### C17 Clarifications Relied Upon
- Well-defined `__has_include` behavior for optional header detection
- Clarified `volatile` semantics
- Deprecated `gets()` formally removed

### C23 Features to Adopt When Available
- `nullptr` instead of `NULL` — type-safe null pointer constant
- `typeof` / `typeof_unqual` — type-safe macros without `__typeof__`
- `constexpr` for true compile-time constants
- `#embed` for binary resource inclusion (replaces `xxd` workflows)
- `[[nodiscard]]`, `[[maybe_unused]]` — standard attributes replacing `__attribute__`
- `bool`, `true`, `false` as keywords (no `<stdbool.h>` needed)
- `<stdbit.h>` — portable bit manipulation

**Migration tip**: Use `#if __STDC_VERSION__ >= 202311L` guards to adopt C23 features incrementally.

---

## Error Handling

### Pattern: Return ErrorCode, Output via Pointer

Every fallible function returns an `ErrorCode` enum. Output values are passed via pointer parameters. This is used consistently across all modules.

```c
// From include/core/error.h
typedef enum {
    ERR_OK = 0,
    ERR_NOMEM,
    ERR_IO,
    ERR_INVALID_ARG,
    ERR_OVERFLOW,
    ERR_NOT_FOUND,
    ERR_UNSUPPORTED,
} ErrorCode;

// Usage pattern (from systems/file_io.h):
ErrorCode file_read_all(const char *path, unsigned char **out_buf, size_t *out_size);
```

### Rules

1. **Always check return values.** Never ignore an `ErrorCode`.
2. **ERR_OK is zero.** This allows `if (err) { handle_error(); }`.
3. **Validate inputs first.** Return `ERR_INVALID_ARG` for NULL pointers or invalid sizes before doing any work.
4. **Use LOG_ERROR_CODE for diagnostics.** The macro captures file and line automatically:
   ```c
   ErrorCode err = arena_init(&a, size);
   if (err) { LOG_ERROR_CODE(err); return err; }
   ```
5. **Propagate errors upward.** Don't swallow errors silently — let callers decide how to handle them.

### Anti-patterns to Avoid

- `errno` as primary error mechanism (fragile, thread-unsafe before C11)
- `setjmp`/`longjmp` for error handling (breaks RAII-like cleanup patterns)
- Returning `-1` or `NULL` without distinguishing error causes

---

## Memory Safety

### Rule 1: Validate All Pointer Parameters

Every function that accepts a pointer checks it before dereferencing:

```c
// From src/memory/arena.c
ErrorCode arena_init(Arena *a, size_t capacity) {
    if (!a || capacity == 0) return ERR_INVALID_ARG;
    // ...
}
```

### Rule 2: Nullify Freed Pointers

After freeing, zero out the struct to prevent use-after-free:

```c
// From src/memory/arena.c
void arena_destroy(Arena *a) {
    if (a) { free(a->buf); a->buf = NULL; a->cap = 0; a->pos = 0; }
}
```

### Rule 3: Prefer Custom Allocators Over Raw malloc

Use arena or pool allocators for predictable allocation patterns:

| Allocator | Use Case | Fragmentation | Speed |
|-----------|----------|---------------|-------|
| Arena     | Bulk alloc, single free point (frames, requests) | None | O(1) bump |
| Pool      | Fixed-size objects with individual free | None | O(1) free-list |
| malloc    | Unpredictable sizes/lifetimes | Possible | Varies |

### Rule 4: Alignment-Aware Allocation

The arena allocator handles alignment explicitly:

```c
// From src/memory/arena.c
size_t aligned = (a->pos + align - 1) & ~(align - 1);
```

Always specify alignment when allocating SIMD vectors or hardware-mapped structures.

### Rule 5: Check Every Allocation

```c
a->buf = malloc(capacity);
if (!a->buf) return ERR_NOMEM;
```

Never assume `malloc` succeeds. In embedded or memory-constrained environments, this is critical.

### Rule 6: Ownership Semantics

Document who owns allocated memory:
- `file_read_all` returns a **caller-freed** buffer (documented in header)
- Arena/pool allocators own their backing memory — destroyed via `_destroy()`
- If a function allocates, its header comment must state who frees

---

## Resource Management

### Pattern: init/destroy Pairs

Every resource type follows the same lifecycle:

```c
Arena a;
ErrorCode err = arena_init(&a, 4096);  // acquire
if (err) { /* handle */ }
// ... use ...
arena_destroy(&a);                      // release
```

### Cleanup on Error Paths

When a function acquires multiple resources, clean up in reverse order on failure:

```c
// From src/systems/file_io.c — file_read_all
FILE *f = fopen(path, "rb");
if (!f) return ERR_IO;

// ... get length ...

*out_buf = malloc((size_t)len);
if (!*out_buf) { fclose(f); return ERR_NOMEM; }  // clean up file

if (fread(*out_buf, 1, (size_t)len, f) != (size_t)len) {
    free(*out_buf); *out_buf = NULL;  // clean up buffer
    fclose(f);                         // clean up file
    return ERR_IO;
}
fclose(f);
```

### Goto-Based Cleanup (Alternative for Complex Functions)

For functions with many resources, a single cleanup label is cleaner than nested ifs:

```c
ErrorCode complex_operation(void) {
    ErrorCode err = ERR_OK;
    FILE *f = NULL;
    void *buf = NULL;

    f = fopen("data.bin", "rb");
    if (!f) { err = ERR_IO; goto cleanup; }

    buf = malloc(4096);
    if (!buf) { err = ERR_NOMEM; goto cleanup; }

    // ... work ...

cleanup:
    free(buf);
    if (f) fclose(f);
    return err;
}
```

This is idiomatic C — the Linux kernel uses this pattern extensively.

---

## API Design

### Header Guard Convention

Use `MODULE_FILENAME_H` format:

```c
#ifndef MEMORY_ARENA_H
#define MEMORY_ARENA_H
// ...
#endif /* MEMORY_ARENA_H */
```

### Opaque vs. Transparent Structs

This template uses **transparent structs** (defined in headers) for simplicity and stack allocation. For library APIs where ABI stability matters, prefer opaque types:

```c
// Opaque (header): users can't see internals
typedef struct Arena Arena;
Arena *arena_create(size_t capacity);

// Transparent (this template): users can stack-allocate
typedef struct Arena { unsigned char *buf; size_t cap, pos; } Arena;
ErrorCode arena_init(Arena *a, size_t capacity);
```

### Naming Conventions

- Types: `PascalCase` (`Arena`, `ErrorCode`, `Pool`)
- Functions: `module_action` (`arena_init`, `pool_alloc`, `file_read_all`)
- Macros: `UPPER_SNAKE` (`LOG_ERROR_CODE`, `ERR_OK`)
- Internal functions: `static` linkage, no prefix needed

### Documentation

Every public function has a Doxygen-compatible doc comment in its header:

```c
/** Read entire file into caller-freed buffer. Sets *out_size to byte count. */
ErrorCode file_read_all(const char *path, unsigned char **out_buf, size_t *out_size);
```

---

## Compiler Warnings & Sanitizers

### Mandatory Warning Flags

From `CMakeLists.txt`:

```cmake
-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wdouble-promotion
-Wformat=2 -Wno-unused-parameter
```

| Flag | Purpose |
|------|---------|
| `-Wall -Wextra` | Broad coverage of common mistakes |
| `-Wpedantic` | Strict ISO C conformance |
| `-Wshadow` | Catches variable shadowing bugs |
| `-Wconversion` | Implicit narrowing conversions |
| `-Wdouble-promotion` | Accidental float→double in printf |
| `-Wformat=2` | Format string vulnerabilities |

### Address Sanitizer

Enable with `-DENABLE_SANITIZERS=ON`:

```bash
cmake -B build -DENABLE_SANITIZERS=ON
cmake --build build
```

This adds `-fsanitize=address,undefined` which catches:
- Buffer overflows (stack, heap, global)
- Use-after-free
- Double-free
- Signed integer overflow
- Null pointer dereference

**Always run tests with sanitizers enabled during development.**

### Treating Warnings as Errors in CI

Add `-Werror` in CI builds to prevent warning regressions:

```cmake
if(CI_BUILD)
    target_compile_options(mylib PRIVATE -Werror)
endif()
```

---

## Portability

### Platform Detection

The template uses `cmake/Platform.cmake` to detect OS and architecture:

- **OS**: `CMAKE_SYSTEM_NAME` → Linux, Darwin (macOS), Windows
- **Architecture**: `CMAKE_SYSTEM_PROCESSOR` → x86_64, aarch64/arm64

### SIMD Portability

Never assume x86. The template provides three tiers:

```c
#if defined(__aarch64__) || defined(_M_ARM64)
    // ARM NEON path
#elif defined(__x86_64__) || defined(_M_X64)
    // SSE/AVX path
#else
    // Scalar fallback — always provide one
#endif
```

### Integer Types

- Use `<stdint.h>` types (`uint32_t`, `int64_t`) for fixed-width data
- Use `size_t` for sizes and indices
- Use `ptrdiff_t` for pointer arithmetic results
- Avoid `int` for sizes — it's only guaranteed 16 bits

### Avoid Platform-Specific APIs in Core Code

Isolate platform-specific code behind abstraction layers (see `systems/` module). Core logic should compile on any conforming C17 implementation.

---

## Concurrency Safety

### Thread Pool Pattern

The template's thread pool (`hpc/thread_pool.h`) demonstrates safe concurrent patterns:

1. **Mutex-protected shared state** — task queue access is serialized
2. **Condition variables for signaling** — workers sleep instead of spinning
3. **Clean shutdown** — set stop flag, broadcast, then join all threads

### Rules for Shared Data

- **No shared mutable state without synchronization.** Period.
- Prefer message passing (task queues) over shared memory
- Use `_Atomic` types for simple flags and counters (C11)
- Use `pthread_mutex_t` for protecting complex data structures
- Avoid lock hierarchies deeper than 2 levels

### Thread-Local Storage

Use `_Thread_local` (C11) or `__thread` (GCC/Clang extension) for per-thread state:

```c
_Thread_local Arena thread_arena;  // each thread gets its own arena
```

---

## Performance Considerations

### Measure Before Optimizing

1. Profile with `perf` (Linux) or Instruments (macOS)
2. Identify actual hotspots — don't guess
3. Optimize the algorithm first, then the implementation

### Data-Oriented Design

- Prefer arrays of structs (AoS) or structs of arrays (SoA) based on access patterns
- Keep hot data contiguous in memory for cache efficiency
- Minimize pointer chasing — the pool allocator's free list is an exception justified by O(1) alloc/free

### Compiler Optimization Levels

| Level | Use Case |
|-------|----------|
| `-O0` | Debugging (default in Debug builds) |
| `-O2` | Release builds — good balance |
| `-O3` | HPC/rendering hotpaths — may increase code size |
| `-Os` | Embedded — optimize for size |
| `-Ofast` | Numerical code — allows non-IEEE float optimizations |

### Restrict Qualifier

Use `restrict` to promise no pointer aliasing, enabling vectorization:

```c
void add_arrays(float *restrict out, const float *restrict a,
                const float *restrict b, size_t n) {
    for (size_t i = 0; i < n; i++) out[i] = a[i] + b[i];
}
```

### Inline Hints

Use `static inline` in headers for small, hot functions. Let the compiler decide for everything else — `inline` is a hint, not a command.

---

## Summary Checklist

- [ ] All functions return `ErrorCode` (or are void/infallible)
- [ ] All pointer parameters validated before use
- [ ] All allocations checked for failure
- [ ] All resources have matching init/destroy calls
- [ ] Freed pointers are nullified
- [ ] Ownership documented in header comments
- [ ] Compiles clean with `-Wall -Wextra -Wpedantic -Wconversion`
- [ ] Tests pass with `-fsanitize=address,undefined`
- [ ] No platform-specific code outside `systems/` or `#ifdef` guards
- [ ] SIMD code has scalar fallback

---

## See Also

- [README](../README.md) — Quick start and module overview
- [TUTORIAL](TUTORIAL.md) — New developer walkthrough
- [ARCHITECTURE](ARCHITECTURE.md) — Project structure and module dependencies
- [TOOLCHAIN](TOOLCHAIN.md) — Required tools and compiler flags
- [EXTENDING](EXTENDING.md) — Adding modules, tests, and dependencies
- [SECURITY_SCANNING](SECURITY_SCANNING.md) — Static analysis and sanitizer details
- [OPTIMIZATION](OPTIMIZATION.md) — Performance tuning
- [CROSS_PLATFORM](CROSS_PLATFORM.md) — Portability guide
