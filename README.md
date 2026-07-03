# C Comprehensive Template

A production-ready C project template covering systems programming, high-performance computing, rendering pipelines, and simulation — with modern best practices (C17).

## Quick Start

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
ctest --output-on-failure
```

## Build Options

| Option | Default | Description |
|---|---|---|
| `BUILD_TESTS` | ON | Build unit tests |
| `BUILD_EXAMPLES` | ON | Build example programs |
| `BUILD_DOCS` | OFF | Build Doxygen documentation |
| `ENABLE_SANITIZERS` | OFF | Enable AddressSanitizer + UBSan |
| `ENABLE_RENDERING` | OFF | Build rendering modules (requires OpenGL) |

## Project Structure

```
├── CMakeLists.txt          Root build configuration
├── AGENTS.md               Agent guide (conventions, build loop, module recipe)
├── cmake/Platform.cmake    Platform & SIMD detection
├── scripts/new_module.sh   Scaffold a new module (files + CMake targets)
├── include/                Public headers
│   ├── core/               Error handling
│   ├── memory/             Arena & pool allocators
│   ├── systems/            File I/O, process control
│   ├── hpc/                SIMD, thread pool, parallel_for
│   ├── rendering/          Software renderer, GL pipeline
│   └── simulation/         Physics, numerical methods
├── src/                    Implementation files
├── tests/                  Unit tests
├── examples/               Working demos
├── docs/                   Doxygen config
└── third_party/            Vendored dependencies
```

## Modules

- **core**: Unified error codes and logging
- **memory**: Arena (bump) allocator and fixed-size pool allocator
- **systems**: Safe file I/O wrappers, process control
- **hpc**: SSE/AVX SIMD operations, pthreads thread pool, parallel_for
- **simulation**: Euler physics integration, Simpson's rule, bisection root finding
- **rendering**: Software framebuffer renderer, OpenGL shader pipeline (optional)

## Documentation

| Document | Description |
|----------|-------------|
| [AGENTS](AGENTS.md) | Condensed guide for AI coding agents (`CLAUDE.md` symlinks here) |
| [ARCHITECTURE](docs/ARCHITECTURE.md) | Project structure, module dependencies, CMake targets |
| [TOOLCHAIN](docs/TOOLCHAIN.md) | Required tools, install instructions, IDE setup |
| [TUTORIAL](docs/TUTORIAL.md) | New developer walkthrough — build, test, extend |
| [EXTENDING](docs/EXTENDING.md) | Adding modules, tests, examples, and dependencies |
| [BEST_PRACTICES](docs/BEST_PRACTICES.md) | Coding standards, error handling, memory safety |
| [CLI](docs/CLI.md) | CLI argument parsing, subcommands, output formatting |
| [SECURITY_SCANNING](docs/SECURITY_SCANNING.md) | Static analysis, sanitizers, CI security gates |
| [OPTIMIZATION](docs/OPTIMIZATION.md) | Profiling, compiler flags, performance patterns |
| [CROSS_PLATFORM](docs/CROSS_PLATFORM.md) | Portability guide — Linux, macOS, Windows |
| [THIRD_PARTY](docs/THIRD_PARTY.md) | Third-party library integration (SDL2, GLFW, cglm, stb) |
