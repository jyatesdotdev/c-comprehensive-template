# Security Scanning

This project integrates multiple security scanning tools into the CMake build process. All tools are optional — they are auto-detected and only enabled when available.

## Quick Start

```bash
# Build with clang-tidy (runs inline during compilation)
cmake -B build -DENABLE_CLANG_TIDY=ON
cmake --build build

# Run cppcheck
cmake -B build && cmake --build build --target cppcheck

# Run all available scanners at once
cmake -B build && cmake --build build --target security-scan

# Build with AddressSanitizer + UBSan
cmake -B build -DENABLE_SANITIZERS=ON
cmake --build build && ctest --test-dir build
```

## Tools

### clang-tidy (Build-Time Static Analysis)

Runs automatically during compilation when enabled. Configuration lives in `.clang-tidy`.

**Enabled checks:**
- `cert-*` — CERT C Coding Standard rules
- `bugprone-*` — common bug patterns (use-after-move, dangling handles, undefined memory ops)
- `clang-analyzer-security.*` — security-specific analysis
- `clang-analyzer-core.*` — null dereference, division by zero, uninitialized values
- `clang-analyzer-unix.*` — POSIX API misuse (malloc/free, file descriptors)
- `clang-analyzer-deadcode.*` — unreachable code
- `clang-analyzer-nullability.*` — nullability annotation violations
- `concurrency-*` — threading and concurrency issues

**Warnings treated as errors:** `cert-*`, `clang-analyzer-security.*`, `bugprone-use-after-move`, `bugprone-dangling-handle`, `bugprone-undefined-memory-manipulation`

**Disabled:** `bugprone-easily-swappable-parameters` (too noisy for C APIs with similar parameter types)

```bash
cmake -B build -DENABLE_CLANG_TIDY=ON -DCMAKE_C_COMPILER=gcc
cmake --build build

# Standalone (without CMake integration)
clang-tidy -p build src/**/*.c
```

### cppcheck (Static Analysis)

Detects undefined behavior, buffer overflows, memory leaks, and portability issues.

Two CMake targets are available:

| Target | Purpose | Output |
|--------|---------|--------|
| `cppcheck` | Interactive use — warnings, performance, portability checks | Console |
| `cppcheck-ci` | CI mode — all checks enabled, stricter | XML (`build/cppcheck-report.xml`) |

Both targets use `--error-exitcode=1` to fail on findings.

```bash
cmake -B build -DENABLE_CPPCHECK=ON
cmake --build build --target cppcheck      # interactive
cmake --build build --target cppcheck-ci   # CI mode with XML report
```

**Suppressions:** `.cppcheck-suppressions` excludes `third_party/*` and suppresses `missingIncludeSystem`.

### AddressSanitizer + UndefinedBehaviorSanitizer

Runtime detection of memory errors and undefined behavior. Enabled via `ENABLE_SANITIZERS`:

```bash
cmake -B build -DENABLE_SANITIZERS=ON
cmake --build build
ctest --test-dir build
```

This adds `-fsanitize=address,undefined -fno-omit-frame-pointer` to all compile and link flags. ASan detects:
- Heap/stack/global buffer overflows
- Use-after-free and use-after-return
- Double-free and invalid free
- Memory leaks (at exit)

UBSan detects:
- Signed integer overflow
- Null pointer dereference
- Misaligned pointer access
- Shift by invalid amount

**Note:** ASan and Valgrind cannot run simultaneously. Use one or the other.

### Valgrind (Memory Error Detection)

Uses CTest's built-in memcheck integration with full leak checking:

```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build
cmake --build build --target valgrind
```

Valgrind runs with `--leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1`.

**Platform note:** Valgrind is Linux-only. The target is only created when `valgrind` is found on the system.

### flawfinder (Source Code Audit)

Scans C source for known dangerous function patterns (e.g., `strcpy`, `sprintf`, `gets`):

```bash
cmake -B build && cmake --build build --target flawfinder
```

Configured with `--minlevel=2` (report level 2+ findings) and `--error-level=4` (exit non-zero on level 4+ findings).

### RATS (Rough Auditing Tool for Security)

Similar to flawfinder — pattern-based source audit:

```bash
cmake -B build && cmake --build build --target rats
```

Configured with `--warning 2` (medium+ severity).

### Aggregate Target

Run all available scanners (cppcheck, flawfinder, rats) with a single command:

```bash
cmake --build build --target security-scan
```

## Suppression Patterns

### clang-tidy

Inline suppression with `NOLINT`:
```c
char buf[10];
strcpy(buf, src);  // NOLINT(clang-analyzer-security.insecureAPI.strcpy)
```

Or suppress for an entire line range:
```c
// NOLINTNEXTLINE(bugprone-narrowing-conversions)
int x = (int)some_double;
```

### cppcheck

Inline suppression:
```c
// cppcheck-suppress arrayIndexOutOfBounds
buf[idx] = val;
```

File-level suppression in `.cppcheck-suppressions`:
```
# Suppress by error ID
uninitvar

# Suppress by error ID and file
arrayIndexOutOfBounds:src/parser.c

# Suppress by error ID, file, and line
memleak:src/alloc.c:42

# Exclude entire directories
*:third_party/*
```

### flawfinder

Inline suppression:
```c
/* Flawfinder: ignore */
strcpy(dst, src);
```

## CI/CD Integration

The project includes `.github/workflows/security.yml` with five parallel jobs:

| Job | Tool | Failure Condition |
|-----|------|-------------------|
| `clang-tidy` | clang-tidy | Any warning in build output |
| `cppcheck` | cppcheck (CI mode) | Any finding (exit code 1) |
| `valgrind` | Valgrind memcheck | Any memory error (exit code 1) |
| `flawfinder` | flawfinder | Level 4+ finding |
| `security-gate` | — | Any upstream job failed |

The `security-gate` job aggregates all results. Add it as a **required status check** in GitHub branch protection to gate merges on security scan results.

### Branch Protection Setup

1. Go to **Settings → Branches → Branch protection rules**
2. Enable **Require status checks to pass before merging**
3. Add `Security gate` as a required check

### Running Locally Before Push

```bash
# Full local security check
cmake -B build -DENABLE_CLANG_TIDY=ON -DENABLE_SANITIZERS=ON -DBUILD_TESTS=ON
cmake --build build
cmake --build build --target security-scan
ctest --test-dir build
```

## CMake Options Reference

| Option | Default | Description |
|--------|---------|-------------|
| `ENABLE_CLANG_TIDY` | `OFF` | Run clang-tidy during build |
| `ENABLE_CPPCHECK` | `OFF` | Enable cppcheck target |
| `ENABLE_SANITIZERS` | `OFF` | Enable ASan + UBSan |
| `BUILD_TESTS` | `ON` | Build tests (required for Valgrind) |

## Files

| File | Purpose |
|------|---------|
| `cmake/Security.cmake` | CMake module — tool detection and target definitions |
| `.clang-tidy` | clang-tidy check configuration |
| `.cppcheck-suppressions` | cppcheck suppression rules |
| `.github/workflows/security.yml` | CI workflow with security gates |

---

## See Also

- [README](../README.md) — Quick start and build options
- [TUTORIAL](TUTORIAL.md) — Walkthrough including running security scans
- [ARCHITECTURE](ARCHITECTURE.md) — CMake targets and build system overview
- [TOOLCHAIN](TOOLCHAIN.md) — Tool installation and versions
- [BEST_PRACTICES](BEST_PRACTICES.md) — Coding standards and sanitizer usage
- [CROSS_PLATFORM](CROSS_PLATFORM.md) — Platform-specific tool availability
