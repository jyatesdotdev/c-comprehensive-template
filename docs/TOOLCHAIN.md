# Toolchain & Development Environment

This document lists every tool the project uses, minimum versions, and installation instructions for macOS and Linux.

## Required Tools

| Tool | Minimum Version | Purpose |
|------|----------------|---------|
| C compiler (GCC or Clang) | GCC 10+ / Clang 14+ | C17 support required |
| CMake | 3.20 | Build system generator |
| Make or Ninja | Any recent | Build backend |
| Git | 2.x | Required by CMake `FetchContent` for third-party deps |

## Optional Tools

| Tool | Purpose | Enabled By |
|------|---------|------------|
| clang-tidy | Static analysis (build-time) | `-DENABLE_CLANG_TIDY=ON` |
| cppcheck | Static analysis (custom target) | Auto-detected; target: `cppcheck` |
| Valgrind | Memory error detection | Auto-detected; target: `valgrind` (Linux only) |
| flawfinder | Source code security audit | Auto-detected; target: `flawfinder` |
| RATS | Security audit | Auto-detected; target: `rats` |
| Doxygen | API documentation generation | `-DBUILD_DOCS=ON` |
| pkg-config | Finds system-installed cmocka | Used when `-DUSE_CMOCKA=ON` |

## Installation

### macOS (Homebrew)

```bash
# Required
xcode-select --install          # Provides Apple Clang (C17-capable)
brew install cmake ninja git

# Optional — static analysis & security scanning
brew install llvm               # For standalone clang-tidy
brew install cppcheck
pip3 install flawfinder

# Optional — documentation
brew install doxygen

# Optional — testing
brew install cmocka
```

> **Note:** Apple Clang (from Xcode CLI tools) supports C17. If you need GCC specifically, install it via `brew install gcc`.

> **Note:** Valgrind does not support macOS on Apple Silicon. Use AddressSanitizer instead: `-DENABLE_SANITIZERS=ON`.

### Linux (Ubuntu/Debian)

```bash
# Required
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build git

# Optional — static analysis & security scanning
sudo apt-get install -y clang-tidy cppcheck valgrind
pip3 install flawfinder

# Optional — documentation
sudo apt-get install -y doxygen

# Optional — testing
sudo apt-get install -y libcmocka-dev pkg-config
```

### Verifying Your Setup

```bash
# Check versions
cc --version            # or gcc --version / clang --version
cmake --version         # Must be >= 3.20
git --version

# Optional tools (no error if missing — CMake will skip them)
clang-tidy --version
cppcheck --version
valgrind --version      # Linux only
flawfinder --version
doxygen --version
```

## Compiler Notes

The project sets these compiler flags in the root `CMakeLists.txt`:

```
-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wdouble-promotion
-Wformat=2 -Wno-unused-parameter -Wnull-dereference
-fstack-protector-strong
```

GCC additionally enables `-Wlogical-op -Wduplicated-cond`.

Sanitizers (ASan + UBSan) are available via `-DENABLE_SANITIZERS=ON` and add:

```
-fsanitize=address,undefined -fno-omit-frame-pointer
```

## IDE Setup

### VS Code

Install these extensions:
- **C/C++** (`ms-vscode.cpptools`) — IntelliSense, debugging
- **CMake Tools** (`ms-vscode.cmake-tools`) — configure/build/test from the editor

Recommended `.vscode/settings.json`:

```json
{
    "cmake.configureArgs": [
        "-DBUILD_TESTS=ON",
        "-DBUILD_EXAMPLES=ON",
        "-DENABLE_CLANG_TIDY=ON"
    ],
    "cmake.generator": "Ninja",
    "C_Cpp.default.cStandard": "c17",
    "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json"
}
```

Generate `compile_commands.json` for full IntelliSense:

```bash
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

### CLion

CLion has built-in CMake support. Open the project root and it will auto-detect `CMakeLists.txt`.

Configure CMake options in **Settings → Build, Execution, Deployment → CMake**:

| Profile | CMake options |
|---------|--------------|
| Debug | `-DBUILD_TESTS=ON -DENABLE_SANITIZERS=ON -DENABLE_CLANG_TIDY=ON` |
| Release | `-DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON` |

CLion will use `compile_commands.json` automatically for code analysis.

## Related Documentation

- [README](../README.md) — Quick start and module overview
- [TUTORIAL](TUTORIAL.md) — New developer walkthrough
- [ARCHITECTURE](ARCHITECTURE.md) — Project structure and CMake target relationships
- [EXTENDING](EXTENDING.md) — Adding modules, tests, and dependencies
- [CLI](CLI.md) — Command-line interface framework
- [SECURITY_SCANNING](SECURITY_SCANNING.md) — Detailed security tool usage and CI integration
- [OPTIMIZATION](OPTIMIZATION.md) — Performance tuning and compiler flags
- [THIRD_PARTY](THIRD_PARTY.md) — Optional dependencies and FetchContent/vcpkg setup
- [CROSS_PLATFORM](CROSS_PLATFORM.md) — Platform detection and portability details
- [BEST_PRACTICES](BEST_PRACTICES.md) — Coding standards and conventions
