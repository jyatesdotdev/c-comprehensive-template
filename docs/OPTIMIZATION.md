# Performance Optimization Guide

## Profiling Tools

### Compiler-Based
```bash
# GCC/Clang profile-guided optimization
gcc -fprofile-generate -o app app.c && ./app
gcc -fprofile-use -o app_opt app.c

# Generate annotated assembly
gcc -O2 -S -fverbose-asm -o app.s app.c
```

### Runtime Profilers
| Tool | Platform | Usage |
|------|----------|-------|
| `perf stat ./app` | Linux | Hardware counters (cache misses, branch mispredicts) |
| `perf record ./app && perf report` | Linux | Sampling profiler |
| Instruments (Time Profiler) | macOS | GUI sampling profiler |
| `valgrind --tool=cachegrind ./app` | Linux/macOS | Cache simulation |

### Sanitizers (Debug Builds)
```cmake
# In CMakeLists.txt — already supported via ENABLE_SANITIZERS
cmake -DENABLE_SANITIZERS=ON ..
```

## Compiler Optimization Flags

```bash
# Optimization levels
-O0   # No optimization (debug)
-O2   # Standard optimization (recommended for production)
-O3   # Aggressive (may increase code size)
-Os   # Optimize for size
-Ofast # -O3 + fast-math (breaks IEEE 754 compliance)

# Architecture-specific
-march=native        # Tune for build machine
-mtune=native        # Schedule for build machine
-msse4.2 / -mavx2   # Explicit ISA (x86)

# Link-time optimization
-flto                # Enable LTO across translation units
```

## Key Optimization Patterns

### 1. Cache-Friendly Access
Access memory sequentially. Row-major traversal of a 2D array is typically 3–10× faster than column-major due to cache line utilization.

```c
/* Good: sequential access */
for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLS; c++)
        sum += matrix[r][c];

/* Bad: strided access — cache miss per element */
for (int c = 0; c < COLS; c++)
    for (int r = 0; r < ROWS; r++)
        sum += matrix[r][c];
```

### 2. Branch Prediction
Sort data before conditional processing when possible. Sorted data gives the branch predictor a consistent pattern.

```c
qsort(data, n, sizeof(int), cmp_int);
for (size_t i = 0; i < n; i++)
    if (data[i] >= threshold) sum += data[i];
```

For hot paths, prefer branchless alternatives:
```c
/* Branchless conditional add */
int mask = -(data[i] >= 128);  /* 0 or -1 */
sum += data[i] & mask;
```

### 3. Custom Allocators
`malloc`/`free` have overhead from bookkeeping and fragmentation. Use arena or pool allocators for known allocation patterns:

- **Arena**: Bulk allocate, reset all at once — ideal for per-frame/per-request data
- **Pool**: Fixed-size blocks with O(1) alloc/free — ideal for uniform objects

See `benchmark_demo.c` for throughput comparison.

### 4. SIMD Vectorization
Process 4+ floats per instruction. The template's `simd_ops.h` auto-selects NEON (ARM) or SSE (x86).

```c
/* Scalar: 1 op/cycle */
for (size_t i = 0; i < n; i++) dst[i] = a[i] + b[i];

/* SIMD: 4 ops/cycle (SSE) or 4 ops/cycle (NEON) */
simd_add_f32(dst, a, b, n);
```

### 5. Restrict and Alignment

```c
/* Tell compiler pointers don't alias */
void add(float *restrict dst, const float *restrict a,
         const float *restrict b, size_t n);

/* Align data for SIMD */
float data[1024] __attribute__((aligned(16)));
```

### 6. Inline Assembly (Architecture-Specific)

```c
/* Read cycle counter (x86) */
static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

/* Memory fence */
__asm__ volatile("" ::: "memory");
```

## Optimization Checklist

1. **Profile first** — never optimize without measurement
2. **Algorithm complexity** — O(n log n) beats O(n²) regardless of micro-optimization
3. **Data layout** — struct-of-arrays for SIMD, cache-friendly traversal
4. **Allocator strategy** — arena/pool for hot paths
5. **Compiler flags** — `-O2 -march=native -flto` for release
6. **SIMD** — vectorize inner loops on float/int arrays
7. **Avoid aliasing** — use `restrict` where applicable
8. **Minimize branches** — branchless code in tight loops
9. **Verify** — re-profile to confirm improvement

---

## See Also

- [README](../README.md) — Quick start and module overview
- [ARCHITECTURE](ARCHITECTURE.md) — Project structure and module dependencies
- [TOOLCHAIN](TOOLCHAIN.md) — Compiler flags and tool versions
- [BEST_PRACTICES](BEST_PRACTICES.md) — Coding standards and performance considerations
- [CROSS_PLATFORM](CROSS_PLATFORM.md) — SIMD portability and platform detection
- [SECURITY_SCANNING](SECURITY_SCANNING.md) — Sanitizers and runtime analysis
