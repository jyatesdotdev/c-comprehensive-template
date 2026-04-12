# Cross-Platform Compatibility Guide

This guide covers patterns and practices for writing portable C code across Linux, macOS, and Windows using this template.

---

## 1. Build System (CMake)

### Platform Detection

The template detects the target OS and architecture in `cmake/Platform.cmake`:

```cmake
# OS detection — sets PLATFORM_LINUX, PLATFORM_MACOS, or PLATFORM_WINDOWS
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM_LINUX TRUE)
    add_compile_definitions(PLATFORM_LINUX)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PLATFORM_MACOS TRUE)
    add_compile_definitions(PLATFORM_MACOS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PLATFORM_WINDOWS TRUE)
    add_compile_definitions(PLATFORM_WINDOWS)
endif()
```

These defines are available in C code for platform-specific branches.

### Compiler-Specific Flags

Not all compilers support the same warning flags. Guard them:

```cmake
# GCC-only flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    add_compile_options(-Wlogical-op -Wduplicated-cond)
endif()

# MSVC requires different flags entirely
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()
```

### Cross-Compilation

```bash
# Build for a different target using a toolchain file
cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-aarch64-linux.cmake
```

Example toolchain file skeleton:

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
```

---

## 2. Portable API Abstractions

### The Pattern: Header with Unified API, Platform-Specific Implementations

The template uses `#ifdef` guards to select platform code. See `src/systems/process.c`:

```c
#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#endif

ErrorCode process_exec(const char *prog, char *const argv[], int *exit_status) {
#ifndef _WIN32
    pid_t pid = fork();
    // ... POSIX implementation
#else
    return ERR_UNSUPPORTED;  // or use CreateProcess()
#endif
}
```

**Best practice**: Keep the public header platform-agnostic. Push all `#ifdef` logic into `.c` files.

### Common POSIX → Windows Mappings

| POSIX | Windows | Notes |
|-------|---------|-------|
| `fork()` / `execvp()` | `CreateProcess()` | Windows has no `fork()` |
| `popen()` | `_popen()` | Prefixed with underscore on MSVC |
| `mmap()` | `CreateFileMapping()` + `MapViewOfFile()` | Different API, same concept |
| `pthread_create()` | `CreateThread()` or `_beginthreadex()` | Or use C11 `<threads.h>` |
| `opendir()` / `readdir()` | `FindFirstFile()` / `FindNextFile()` | Or use C23 `<dirent.h>` proposal |
| `usleep()` / `nanosleep()` | `Sleep()` | Millisecond granularity on Windows |
| `SIGINT` handler | `SetConsoleCtrlHandler()` | `signal()` works for basic cases |

### Recommended Abstraction Layer

For non-trivial projects, create a `platform/` module:

```
include/platform/
    platform.h      // Unified types and macros
    threads.h       // Thread abstraction
    fs.h            // Filesystem abstraction
    time.h          // High-resolution timing
```

---

## 3. Data Type Portability

### Always Use Fixed-Width Types

```c
#include <stdint.h>   // int32_t, uint64_t, etc.
#include <stddef.h>   // size_t, ptrdiff_t
#include <stdbool.h>  // bool (C99+; built-in in C23)
```

Avoid `int` for sizes or indices — use `size_t`. Avoid `long` — its size varies (4 bytes on Windows x64, 8 bytes on Linux x64).

### Struct Packing and Alignment

Compilers pad structs differently. For binary-compatible layouts:

```c
// GCC/Clang
typedef struct __attribute__((packed)) {
    uint8_t  type;
    uint32_t value;
} PackedMsg;

// MSVC
#pragma pack(push, 1)
typedef struct {
    uint8_t  type;
    uint32_t value;
} PackedMsg;
#pragma pack(pop)

// Portable macro
#if defined(_MSC_VER)
    #define PACKED_BEGIN __pragma(pack(push, 1))
    #define PACKED_END   __pragma(pack(pop))
    #define PACKED_ATTR
#else
    #define PACKED_BEGIN
    #define PACKED_END
    #define PACKED_ATTR __attribute__((packed))
#endif
```

### Endianness

```c
#include <stdint.h>

static inline uint32_t swap32(uint32_t v) {
    return ((v >> 24) & 0xFF)
         | ((v >>  8) & 0xFF00)
         | ((v <<  8) & 0xFF0000)
         | ((v << 24) & 0xFF000000);
}

// Use compiler builtins when available
#if defined(__GNUC__) || defined(__clang__)
    #define BSWAP32(x) __builtin_bswap32(x)
    #define BSWAP64(x) __builtin_bswap64(x)
#elif defined(_MSC_VER)
    #include <stdlib.h>
    #define BSWAP32(x) _byteswap_ulong(x)
    #define BSWAP64(x) _byteswap_uint64(x)
#else
    #define BSWAP32(x) swap32(x)
#endif
```

---

## 4. SIMD Portability

The template handles SIMD via compile-time detection in `cmake/Platform.cmake` and `#ifdef` chains in `src/hpc/simd_ops.c`:

```c
#if defined(HAS_SSE42) || defined(HAS_AVX2)
#include <immintrin.h>
#elif defined(HAS_NEON)
#include <arm_neon.h>
#endif

void simd_add_f32(float *dst, const float *a, const float *b, size_t n) {
    size_t i = 0;
#if defined(HAS_NEON)
    for (; i + 4 <= n; i += 4)
        vst1q_f32(dst + i, vaddq_f32(vld1q_f32(a + i), vld1q_f32(b + i)));
#elif defined(HAS_SSE42)
    for (; i + 4 <= n; i += 4)
        _mm_storeu_ps(dst + i, _mm_add_ps(_mm_loadu_ps(a + i), _mm_loadu_ps(b + i)));
#endif
    for (; i < n; i++) dst[i] = a[i] + b[i];  // scalar fallback — always present
}
```

**Key rules**:
- Always provide a scalar fallback after SIMD loops
- Detect capabilities at build time (CMake) rather than runtime for simplicity
- Process remaining elements (tail loop) after the SIMD loop

---

## 5. Threading Portability

### Option A: pthreads (POSIX + Windows via pthreads-win32)

This template uses pthreads. On Windows, link against `pthreads-win32` or `winpthreads` (MinGW).

### Option B: C11 `<threads.h>`

C11 provides a portable threading API. Support varies:
- GCC/glibc: Supported since glibc 2.28
- Clang/musl: Supported
- MSVC: Supported since VS 2022 17.8

```c
#include <threads.h>

int worker(void *arg) {
    // ...
    return 0;
}

thrd_t t;
thrd_create(&t, worker, NULL);
thrd_join(t, NULL);
```

### Option C: Thin Abstraction

```c
// platform/threads.h
#ifdef _WIN32
    #include <windows.h>
    typedef HANDLE thread_t;
    typedef DWORD (WINAPI *thread_fn)(void *);
#else
    #include <pthread.h>
    typedef pthread_t thread_t;
    typedef void *(*thread_fn)(void *);
#endif

int thread_create(thread_t *t, thread_fn fn, void *arg);
int thread_join(thread_t t);
```

---

## 6. File Path Handling

```c
// Path separator
#ifdef _WIN32
    #define PATH_SEP '\\'
    #define PATH_SEP_STR "\\"
#else
    #define PATH_SEP '/'
    #define PATH_SEP_STR "/"
#endif

// Max path length
#ifdef _WIN32
    #define MAX_PATH_LEN 260   // MAX_PATH
#else
    #include <limits.h>
    #define MAX_PATH_LEN PATH_MAX  // typically 4096
#endif
```

Note: Windows also accepts `/` as a separator in most APIs. Use `/` in CMake and config files.

---

## 7. High-Resolution Timing

```c
#ifdef _WIN32
    #include <windows.h>
    static inline double get_time_sec(void) {
        LARGE_INTEGER freq, count;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&count);
        return (double)count.QuadPart / (double)freq.QuadPart;
    }
#else
    #include <time.h>
    static inline double get_time_sec(void) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec + ts.tv_nsec * 1e-9;
    }
#endif
```

The template's `testing/perf_test.h` uses `clock_gettime()` — wrap it for Windows portability.

---

## 8. Compiler Compatibility Macros

```c
// Function attributes
#if defined(__GNUC__) || defined(__clang__)
    #define ATTR_UNUSED      __attribute__((unused))
    #define ATTR_NORETURN    __attribute__((noreturn))
    #define ATTR_PRINTF(f,a) __attribute__((format(printf, f, a)))
    #define LIKELY(x)        __builtin_expect(!!(x), 1)
    #define UNLIKELY(x)      __builtin_expect(!!(x), 0)
#elif defined(_MSC_VER)
    #define ATTR_UNUSED
    #define ATTR_NORETURN    __declspec(noreturn)
    #define ATTR_PRINTF(f,a)
    #define LIKELY(x)        (x)
    #define UNLIKELY(x)      (x)
#else
    #define ATTR_UNUSED
    #define ATTR_NORETURN
    #define ATTR_PRINTF(f,a)
    #define LIKELY(x)        (x)
    #define UNLIKELY(x)      (x)
#endif

// DLL export/import (for shared libraries)
#ifdef _WIN32
    #define API_EXPORT __declspec(dllexport)
    #define API_IMPORT __declspec(dllimport)
#else
    #define API_EXPORT __attribute__((visibility("default")))
    #define API_IMPORT
#endif
```

---

## 9. C Standard Portability

| Feature | C11 | C17 | C23 |
|---------|-----|-----|-----|
| `_Static_assert` | ✓ | ✓ | `static_assert` (no underscore) |
| `_Alignas` / `_Alignof` | ✓ | ✓ | `alignas` / `alignof` |
| `<threads.h>` | ✓ | ✓ | ✓ |
| `<stdatomic.h>` | ✓ | ✓ | ✓ |
| `typeof` | Extension | Extension | ✓ (standard) |
| `nullptr` | No | No | ✓ |
| `bool` built-in | No (`<stdbool.h>`) | No | ✓ |
| `#embed` | No | No | ✓ |
| Attributes `[[...]]` | No | No | ✓ |

The template targets C17 (`CMAKE_C_STANDARD 17`). For C23 features, use feature-test macros:

```c
#if __STDC_VERSION__ >= 202311L
    // C23 available
    static_assert(sizeof(int) >= 4);
#elif __STDC_VERSION__ >= 201112L
    // C11/C17
    _Static_assert(sizeof(int) >= 4, "int must be at least 32 bits");
#endif
```

---

## 10. CI/CD Matrix

Test across platforms in CI. Example GitHub Actions matrix:

```yaml
strategy:
  matrix:
    include:
      - os: ubuntu-latest
        cc: gcc
      - os: ubuntu-latest
        cc: clang
      - os: macos-latest
        cc: clang
      - os: windows-latest
        cc: cl

steps:
  - uses: actions/checkout@v4
  - name: Configure
    run: cmake -B build -DCMAKE_C_COMPILER=${{ matrix.cc }}
  - name: Build
    run: cmake --build build
  - name: Test
    run: ctest --test-dir build --output-on-failure
```

---

## Quick Checklist

- [ ] Use `<stdint.h>` fixed-width types for all serialized/binary data
- [ ] Provide scalar fallbacks for all SIMD code paths
- [ ] Guard POSIX-only APIs with `#ifndef _WIN32`
- [ ] Use CMake's `find_package()` and generator expressions for platform deps
- [ ] Test with at least GCC, Clang, and MSVC
- [ ] Avoid `long` — use `int32_t`/`int64_t` or `size_t`
- [ ] Handle path separators if constructing paths at runtime
- [ ] Use `clock_gettime()` / `QueryPerformanceCounter()` for timing, not `clock()`

---

## See Also

- [README](../README.md) — Quick start and module overview
- [ARCHITECTURE](ARCHITECTURE.md) — Project structure and CMake target relationships
- [TOOLCHAIN](TOOLCHAIN.md) — Required tools and platform install instructions
- [BEST_PRACTICES](BEST_PRACTICES.md) — Coding standards and portability rules
- [OPTIMIZATION](OPTIMIZATION.md) — Performance tuning and SIMD usage
- [SECURITY_SCANNING](SECURITY_SCANNING.md) — Static analysis and CI integration
