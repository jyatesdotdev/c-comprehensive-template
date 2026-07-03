# Architecture

## Project Structure

```
c_comprehensive_template/
├── CMakeLists.txt              Root build configuration
├── Doxyfile.in                 Doxygen config template
├── AGENTS.md                   Agent guide (CLAUDE.md symlinks here)
├── .clang-format               Enforced formatting style (checked in CI)
├── .clang-tidy                 clang-tidy rules
├── .cppcheck-suppressions      cppcheck suppression list
├── .github/workflows/
│   └── security.yml            CI security scanning workflow
├── cmake/
│   ├── Platform.cmake          OS & SIMD detection (Linux/macOS/Windows, SSE/AVX/NEON)
│   ├── Security.cmake          Static analysis & security tool targets
│   ├── Testing.cmake           Unity & cmocka test framework integration
│   └── ThirdParty.cmake        Optional dependencies via FetchContent
├── include/                    Public headers (all modules)
│   ├── core/error.h
│   ├── memory/{arena,pool,leak_detect}.h
│   ├── systems/{file_io,process}.h
│   ├── hpc/{simd_ops,thread_pool,parallel}.h
│   ├── rendering/{software_renderer,gl_pipeline,vk_pipeline}.h
│   ├── simulation/{physics,numerical}.h
│   └── testing/perf_test.h
├── src/                        Implementation files (mirrors include/)
├── scripts/
│   └── new_module.sh           Scaffold a new module (files + CMake registration)
├── tests/                      Unit & performance tests
├── examples/                   Working demo programs
├── docs/                       Documentation & Doxygen build
└── third_party/                Vendored headers (e.g. stb)
```

## Module Dependency Graph

All library modules are built as static libraries. Arrows indicate `target_link_libraries` dependencies.

```
                    ┌──────────┐
                    │   core   │  (error.c, arena.c, pool.c, leak_detect.c)
                    └────┬─────┘
           ┌─────────┬───┼───────┬──────────────┐
           │         │   │       │              │
           ▼         ▼   │       ▼              ▼
       ┌──────┐  ┌──────┐│  ┌──────────┐  ┌────────────┐
       │ cli  │  │systems││  │simulation│  │rendering_sw│
       └──────┘  └──────┘│  └──────────┘  └────────────┘
                          │       │
                          ▼       │  (links libm)
                     ┌────────┐   │
                     │  hpc   │───┘
                     └────────┘
                          │
                     (links pthreads)

       ┌───────────┐
       │ rendering  │  (optional, ENABLE_RENDERING=ON)
       └───────────┘   gl_pipeline.c + vk_pipeline.c
            │
            └── links core (+ OpenGL/Vulkan via Platform.cmake)
```

Key relationships:
- `core` is the foundation — every other module depends on it
- `cli` depends on `core` (uses error handling)
- `systems` depends on `core` (file I/O, process wrappers)
- `hpc` depends on `core` + `Threads::Threads` (pthreads)
- `simulation` depends on `core` + `m` (libm for math)
- `rendering_sw` depends on `core` (software renderer, always built)
- `rendering` depends on `core` (optional GL/Vulkan, requires `ENABLE_RENDERING=ON`)

## Build System Overview

The build uses CMake 3.20+ with C17. The root `CMakeLists.txt` orchestrates everything:

### Build Flow

1. **Compiler setup** — C17 standard, strict warnings (`-Wall -Wextra -Wpedantic -Wshadow` etc.), stack protector
2. **cmake/Security.cmake** — Included early so `CMAKE_C_CLANG_TIDY` applies to all subsequent targets
3. **cmake/Platform.cmake** — Detects OS (Linux/macOS/Windows) and architecture (x86 SSE/AVX, ARM NEON), sets compile definitions
4. **cmake/ThirdParty.cmake** — Optional FetchContent dependencies (SDL2, GLFW, cglm, cJSON, argtable3), plus vendored stb headers
5. **Library targets** — `core`, `cli`, `systems`, `hpc`, `simulation`, `rendering_sw`, and optionally `rendering`
6. **cmake/Testing.cmake** — Fetches Unity test framework, optionally finds cmocka
7. **tests/** — Test executables linked against library targets + test frameworks
8. **examples/** — Demo executables linked against library targets
9. **docs/** — Doxygen documentation generation (when `BUILD_DOCS=ON`)

### CMake Options

| Option               | Default | Effect                                          |
|----------------------|---------|-------------------------------------------------|
| `BUILD_TESTS`        | ON      | Build test executables, enable CTest            |
| `BUILD_EXAMPLES`     | ON      | Build example programs                          |
| `BUILD_DOCS`         | OFF     | Build Doxygen HTML docs                         |
| `ENABLE_SANITIZERS`  | OFF     | Add `-fsanitize=address,undefined`              |
| `ENABLE_RENDERING`   | OFF     | Build GL/Vulkan rendering module                |
| `ENABLE_CLANG_TIDY`  | OFF     | Run clang-tidy during compilation               |
| `ENABLE_CPPCHECK`    | OFF     | Enable cppcheck target                          |
| `USE_UNITY`          | ON      | Fetch Unity test framework                      |
| `USE_CMOCKA`         | OFF     | Use system-installed cmocka                     |
| `USE_SDL2`           | OFF     | Fetch/find SDL2                                 |
| `USE_GLFW`           | OFF     | Fetch/find GLFW                                 |
| `USE_CGLM`           | OFF     | Fetch cglm math library                         |
| `USE_CJSON`          | OFF     | Fetch cJSON                                     |
| `USE_ARGTABLE3`      | OFF     | Fetch argtable3 CLI parser                      |

### Custom Build Targets

These targets are defined in `cmake/Security.cmake` and available when the corresponding tools are installed:

| Target          | Tool       | Description                              |
|-----------------|------------|------------------------------------------|
| `cppcheck`      | cppcheck   | Static analysis (warning, performance, portability) |
| `cppcheck-ci`   | cppcheck   | Stricter analysis with XML output        |
| `valgrind`      | Valgrind   | Memory error detection via CTest memcheck |
| `flawfinder`    | flawfinder | Source code security audit               |
| `rats`          | RATS       | Rough Auditing Tool for Security         |
| `security-scan` | (aggregate)| Runs all available security scan targets |

### Test Targets

Tests are organized by framework:

| Test Executable       | Framework   | Libraries          | What it tests              |
|-----------------------|-------------|--------------------|----------------------------|
| `test_arena`          | minimal     | core               | Arena allocator             |
| `test_pool`           | minimal     | core               | Pool allocator              |
| `test_leak_detect`    | minimal     | core               | Leak detection              |
| `test_hpc`            | minimal     | core, hpc, m       | SIMD, thread pool, parallel |
| `test_simulation`     | minimal     | core, simulation, m| Physics, numerical methods  |
| `test_memory_unity`   | Unity       | core               | Memory (Unity framework)    |
| `test_cli`            | Unity       | cli, core          | CLI argument parsing        |
| `test_memory_cmocka`  | cmocka      | core               | Memory (cmocka framework)   |
| `test_perf_memory`    | perf_test.h | core               | Memory performance benchmarks|

### Example Targets

| Example Executable       | Libraries              | Demo                        |
|--------------------------|------------------------|-----------------------------|
| `example_arena`          | core                   | Arena allocator usage        |
| `example_thread_pool`    | core, hpc              | Thread pool usage            |
| `example_physics`        | core, simulation       | Physics integration          |
| `example_systems`        | core, systems          | File I/O, process control    |
| `example_hpc`            | core, hpc, m           | SIMD + parallel_for          |
| `example_rendering`      | core, rendering_sw     | Software framebuffer         |
| `example_simulation`     | core, simulation, m    | Numerical methods            |
| `example_benchmark`      | core, hpc, m           | Performance benchmarking     |
| `example_cli`            | cli, core              | CLI argument parsing         |
| `example_cli_argtable`   | argtable3              | argtable3 CLI (optional)     |

## See Also

- [README](../README.md) — Quick start and module overview
- [TUTORIAL](TUTORIAL.md) — New developer walkthrough
- [TOOLCHAIN](TOOLCHAIN.md) — Required tools and installation
- [EXTENDING](EXTENDING.md) — How to add modules, tests, and dependencies
- [CLI](CLI.md) — Command-line interface framework
- [BEST_PRACTICES](BEST_PRACTICES.md) — Coding standards
- [SECURITY_SCANNING](SECURITY_SCANNING.md) — Security tool details
- [THIRD_PARTY](THIRD_PARTY.md) — Third-party library integration
- [CROSS_PLATFORM](CROSS_PLATFORM.md) — Platform support details
- [OPTIMIZATION](OPTIMIZATION.md) — Performance tuning
