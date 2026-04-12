# Getting Started Tutorial

A hands-on walkthrough for new developers: clone, build, test, run examples, add a feature, and run security scans.

> **Prerequisites:** See [TOOLCHAIN](TOOLCHAIN.md) for required tools and installation instructions.

---

## 1. Clone and Build

```bash
git clone <repository-url>
cd c_comprehensive_template

# Configure and build (Debug mode for development)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

This builds all libraries (`core`, `cli`, `systems`, `hpc`, `simulation`, `rendering_sw`), tests, and examples by default.

### Common Build Variants

```bash
# Release build with optimizations
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# With sanitizers (catches memory bugs at runtime)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build -j$(nproc)

# With Unity test framework
cmake -B build -DUSE_UNITY=ON
cmake --build build -j$(nproc)
```

See the [README](../README.md) for all build options.

---

## 2. Run Tests

```bash
# Run all tests
ctest --test-dir build --output-on-failure

# Run a specific test
ctest --test-dir build -R test_arena --output-on-failure

# List available tests
ctest --test-dir build -N
```

Expected output:

```
test_arena: PASS
test_pool: PASS
test_leak_detect: PASS
test_hpc: PASS
test_simulation: PASS
test_perf_memory: PASS
```

---

## 3. Run an Example

All example binaries are in `build/examples/`:

```bash
# Arena allocator demo
./build/examples/example_arena

# Physics simulation
./build/examples/example_physics

# SIMD and threading
./build/examples/example_hpc

# CLI argument parsing
./build/examples/example_cli --name world --count 3
```

See `examples/` for source code. Each demo is self-contained and shows one module in action.

---

## 4. Add a Feature

Let's add a simple `stack` data structure to the `core` module. This walks through the full cycle: header, implementation, test, example.

### 4a. Create the Header

Create `include/core/stack.h`:

```c
/**
 * @file stack.h
 * @brief Simple fixed-capacity integer stack.
 */
#ifndef CORE_STACK_H
#define CORE_STACK_H

#include "core/error.h"
#include <stddef.h>

/** @brief Fixed-capacity stack of integers. */
typedef struct {
    int *data;
    size_t capacity;
    size_t count;
} Stack;

/**
 * @brief Initialize a stack with the given capacity.
 * @param s Pointer to stack (must not be NULL).
 * @param capacity Maximum number of elements.
 * @return ERR_OK on success, ERR_NOMEM if allocation fails.
 */
ErrorCode stack_init(Stack *s, size_t capacity);

/**
 * @brief Push a value onto the stack.
 * @param s Pointer to an initialized stack.
 * @param value Value to push.
 * @return ERR_OK on success, ERR_OVERFLOW if full.
 */
ErrorCode stack_push(Stack *s, int value);

/**
 * @brief Pop a value from the stack.
 * @param s Pointer to an initialized stack.
 * @param out Receives the popped value (must not be NULL).
 * @return ERR_OK on success, ERR_NOT_FOUND if empty.
 */
ErrorCode stack_pop(Stack *s, int *out);

/** @brief Free stack resources. */
void stack_destroy(Stack *s);

#endif /* CORE_STACK_H */
```

### 4b. Create the Implementation

Create `src/core/stack.c`:

```c
#include "core/stack.h"
#include <stdlib.h>

ErrorCode stack_init(Stack *s, size_t capacity) {
    s->data = malloc(capacity * sizeof(int));
    if (!s->data) return ERR_NOMEM;
    s->capacity = capacity;
    s->count = 0;
    return ERR_OK;
}

ErrorCode stack_push(Stack *s, int value) {
    if (s->count >= s->capacity) return ERR_OVERFLOW;
    s->data[s->count++] = value;
    return ERR_OK;
}

ErrorCode stack_pop(Stack *s, int *out) {
    if (s->count == 0) return ERR_NOT_FOUND;
    *out = s->data[--s->count];
    return ERR_OK;
}

void stack_destroy(Stack *s) {
    free(s->data);
    s->data = NULL;
    s->count = 0;
}
```

### 4c. Register in CMake

Add `src/core/stack.c` to the `core` library in the root `CMakeLists.txt`:

```cmake
add_library(core STATIC
    src/core/error.c
    src/core/stack.c       # ← add this line
    src/memory/arena.c
    src/memory/pool.c
    src/memory/leak_detect.c
)
```

### 4d. Write a Test

Create `tests/test_stack.c`:

```c
#include <stdio.h>
#include <stdlib.h>
#include "core/stack.h"

#define ASSERT(cond) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); exit(1); } \
} while(0)

int main(void) {
    Stack s;
    ASSERT(stack_init(&s, 4) == ERR_OK);
    ASSERT(stack_push(&s, 10) == ERR_OK);
    ASSERT(stack_push(&s, 20) == ERR_OK);

    int val;
    ASSERT(stack_pop(&s, &val) == ERR_OK);
    ASSERT(val == 20);

    /* Fill to capacity */
    ASSERT(stack_push(&s, 30) == ERR_OK);
    ASSERT(stack_push(&s, 40) == ERR_OK);
    ASSERT(stack_push(&s, 50) == ERR_OVERFLOW);

    stack_destroy(&s);
    printf("test_stack: PASS\n");
    return 0;
}
```

Register it in `tests/CMakeLists.txt`:

```cmake
add_executable(test_stack test_stack.c)
target_link_libraries(test_stack PRIVATE core)
add_test(NAME test_stack COMMAND test_stack)
```

### 4e. Build and Verify

```bash
cmake --build build -j$(nproc)
ctest --test-dir build -R test_stack --output-on-failure
```

For a complete guide on adding modules, examples, and dependencies, see [EXTENDING](EXTENDING.md).

---

## 5. Run Security Scans

The project includes several integrated security tools. See [SECURITY_SCANNING](SECURITY_SCANNING.md) for full details.

### Static Analysis with clang-tidy

```bash
cmake -B build -DENABLE_CLANG_TIDY=ON
cmake --build build
```

clang-tidy runs automatically during compilation and checks for CERT C violations, null dereferences, buffer overflows, and more.

### Static Analysis with cppcheck

```bash
cmake -B build -DENABLE_CPPCHECK=ON
cmake --build build --target cppcheck
```

### Runtime Analysis with Sanitizers

```bash
cmake -B build -DENABLE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

AddressSanitizer catches buffer overflows, use-after-free, and memory leaks. UndefinedBehaviorSanitizer catches signed overflow, null dereferences, and misaligned access.

### Memory Checking with Valgrind (Linux only)

```bash
cmake --build build --target valgrind
```

### Run All Scanners

```bash
cmake -B build -DENABLE_CLANG_TIDY=ON -DENABLE_CPPCHECK=ON -DENABLE_SANITIZERS=ON
cmake --build build
cmake --build build --target security-scan
ctest --test-dir build --output-on-failure
```

---

## 6. Generate Documentation

```bash
cmake -B build -DBUILD_DOCS=ON
cmake --build build --target docs
```

This generates Doxygen HTML documentation from the in-code `@brief`, `@param`, and `@return` comments. Output goes to `build/docs/html/`.

---

## Next Steps

- [ARCHITECTURE](ARCHITECTURE.md) — project structure and module dependencies
- [TOOLCHAIN](TOOLCHAIN.md) — tool versions and IDE setup
- [EXTENDING](EXTENDING.md) — adding modules, tests, and dependencies
- [BEST_PRACTICES](BEST_PRACTICES.md) — coding standards and patterns
- [SECURITY_SCANNING](SECURITY_SCANNING.md) — full security tool reference
- [CLI](CLI.md) — argument parsing framework
- [OPTIMIZATION](OPTIMIZATION.md) — performance tuning
- [CROSS_PLATFORM](CROSS_PLATFORM.md) — portability guide
