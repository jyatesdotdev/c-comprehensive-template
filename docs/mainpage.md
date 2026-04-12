# C Comprehensive Template {#mainpage}

A production-ready C project template covering systems programming, high-performance computing, rendering pipelines, and simulation.

## Modules

| Module | Description |
|--------|-------------|
| @ref core/error.h "core" | Unified error codes and logging |
| @ref memory/arena.h "memory/arena" | Bump allocator for bulk-freed allocations |
| @ref memory/pool.h "memory/pool" | Fixed-size block pool allocator |
| @ref memory/leak_detect.h "memory/leak_detect" | Debug allocator with leak reporting |
| @ref systems/file_io.h "systems/file_io" | File I/O, memory-mapped files, directory walking |
| @ref systems/process.h "systems/process" | Process control, fork/exec, signal handling |
| @ref hpc/simd_ops.h "hpc/simd" | SIMD operations (NEON/SSE with scalar fallback) |
| @ref hpc/thread_pool.h "hpc/thread_pool" | Pthreads-based thread pool |
| @ref hpc/parallel.h "hpc/parallel" | Parallel for-each and map-reduce |
| @ref rendering/software_renderer.h "rendering/sw" | Software rasterizer (lines, rects, circles, PPM) |
| @ref rendering/gl_pipeline.h "rendering/gl" | OpenGL shader pipeline |
| @ref rendering/vk_pipeline.h "rendering/vk" | Vulkan pipeline setup |
| @ref simulation/physics.h "simulation/physics" | Verlet integration, collisions, springs |
| @ref simulation/numerical.h "simulation/numerical" | Root finding, ODE solvers, quadrature |

## Building Documentation

```bash
cmake -B build -DBUILD_DOCS=ON
cmake --build build --target docs
open build/docs/html/index.html
```

## Quick Start

```bash
cmake -B build -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build
```

## Further Reading

- [Tutorial](TUTORIAL.md) — New developer walkthrough
- [Architecture](ARCHITECTURE.md) — Project structure and module dependencies
- [Toolchain](TOOLCHAIN.md) — Required tools and IDE setup
- [Extending](EXTENDING.md) — Adding modules, tests, and dependencies
- [Best Practices](BEST_PRACTICES.md) — Coding standards
- [CLI](CLI.md) — Command-line interface framework
- [Security Scanning](SECURITY_SCANNING.md) — Static analysis and CI gates
- [Optimization](OPTIMIZATION.md) — Performance tuning
- [Cross-Platform](CROSS_PLATFORM.md) — Portability guide
- [Third-Party](THIRD_PARTY.md) — Library integration
