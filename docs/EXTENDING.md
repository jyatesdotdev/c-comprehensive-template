# Extending the Template

This guide covers how to add new modules, examples, tests, and third-party dependencies to the project.

> See also: [ARCHITECTURE](ARCHITECTURE.md) for project structure, [TOOLCHAIN](TOOLCHAIN.md) for build prerequisites, [THIRD_PARTY](THIRD_PARTY.md) for existing dependency details.

---

## Adding a New Library Module

Modules follow the convention: headers in `include/<module>/`, sources in `src/<module>/`.

### 1. Create the header

```c
/* include/networking/socket.h */

/**
 * @file socket.h
 * @brief Cross-platform socket abstraction.
 */
#ifndef NETWORKING_SOCKET_H
#define NETWORKING_SOCKET_H

#include "core/error.h"

/**
 * @brief Open a TCP connection.
 * @param host Hostname or IP address.
 * @param port Port number.
 * @return ERR_OK on success, or an error code.
 */
ErrorCode socket_connect(const char *host, int port);

#endif /* NETWORKING_SOCKET_H */
```

### 2. Create the source

```c
/* src/networking/socket.c */
#include "networking/socket.h"
#include <stdio.h>

ErrorCode socket_connect(const char *host, int port) {
    if (!host || port <= 0) return ERR_INVALID_ARG;
    printf("Connecting to %s:%d\n", host, port);
    return ERR_OK;
}
```

### 3. Register the library target in `CMakeLists.txt`

Add a new section in the root `CMakeLists.txt`, following the existing pattern:

```cmake
# ── Networking Library ──────────────────────────────────────────────────────
add_library(networking STATIC
    src/networking/socket.c
)
target_include_directories(networking PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(networking PRIVATE core)
```

Place it after the existing library blocks (core, cli, systems, hpc, simulation, rendering).

### 4. Build and verify

```bash
cmake --build build
```

The new library is now available for linking by tests and examples via `target_link_libraries(... PRIVATE networking)`.

---

## Adding a New Example

Examples live in `examples/` and are registered in `examples/CMakeLists.txt`.

### 1. Create the source file

```c
/* examples/networking_demo.c */
#include "networking/socket.h"
#include <stdio.h>

int main(void) {
    ErrorCode err = socket_connect("localhost", 8080);
    printf("Result: %s\n", error_str(err));
    return 0;
}
```

### 2. Register in `examples/CMakeLists.txt`

```cmake
add_executable(example_networking ../examples/networking_demo.c)
target_link_libraries(example_networking PRIVATE core networking)
```

### 3. Build and run

```bash
cmake --build build
./build/examples/example_networking
```

Examples are only built when `BUILD_EXAMPLES=ON` (the default).

---

## Adding a New Test

Tests live in `tests/` and are registered in `tests/CMakeLists.txt`. The project supports three test backends:

| Backend | When available | Link target |
|---------|---------------|-------------|
| Minimal (built-in) | Always | — (just `assert.h`) |
| Unity | `USE_UNITY=ON` (default) | `unity::framework` |
| cmocka | `USE_CMOCKA=ON` + system install | `cmocka::cmocka` |

### Minimal test (no framework)

```c
/* tests/test_networking.c */
#include "networking/socket.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    assert(socket_connect(NULL, 80) == ERR_INVALID_ARG);
    assert(socket_connect("localhost", -1) == ERR_INVALID_ARG);
    assert(socket_connect("localhost", 80) == ERR_OK);
    printf("All networking tests passed.\n");
    return 0;
}
```

Register in `tests/CMakeLists.txt`:

```cmake
add_executable(test_networking test_networking.c)
target_link_libraries(test_networking PRIVATE core networking)
add_test(NAME test_networking COMMAND test_networking)
```

### Unity test

Wrap tests in the Unity framework for richer assertions and output:

```c
/* tests/test_networking_unity.c */
#include "unity.h"
#include "networking/socket.h"

void setUp(void) {}
void tearDown(void) {}

void test_connect_null_host(void) {
    TEST_ASSERT_EQUAL(ERR_INVALID_ARG, socket_connect(NULL, 80));
}

void test_connect_valid(void) {
    TEST_ASSERT_EQUAL(ERR_OK, socket_connect("localhost", 80));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_connect_null_host);
    RUN_TEST(test_connect_valid);
    return UNITY_END();
}
```

Register (guarded by the Unity target):

```cmake
if(TARGET unity::framework)
    add_executable(test_networking_unity test_networking_unity.c)
    target_link_libraries(test_networking_unity PRIVATE unity::framework core networking)
    add_test(NAME test_networking_unity COMMAND test_networking_unity)
endif()
```

### Running tests

```bash
cd build && ctest --output-on-failure
```

---

## Adding a Third-Party Dependency

Third-party libraries are managed in `cmake/ThirdParty.cmake`. The project uses two strategies:

### Strategy 1: FetchContent (preferred for source-available libraries)

Add a new block in `cmake/ThirdParty.cmake`:

```cmake
# ── libcurl (HTTP client) ──────────────────────────────────────────────────
option(USE_CURL "Use libcurl for HTTP" OFF)
if(USE_CURL)
    find_package(CURL QUIET)
    if(NOT CURL_FOUND)
        FetchContent_Declare(curl
            GIT_REPOSITORY https://github.com/curl/curl.git
            GIT_TAG        curl-8_6_0
            GIT_SHALLOW    TRUE
        )
        FetchContent_MakeAvailable(curl)
    endif()
    message(STATUS "libcurl enabled")
endif()
```

Key conventions:
- Always gate behind an `option(USE_<NAME> ...)` so it's opt-in
- Try `find_package()` first to use a system install if available
- Use `GIT_SHALLOW TRUE` to speed up clones
- Disable the dependency's tests/examples/docs via cache variables

Enable at configure time:

```bash
cmake -B build -DUSE_CURL=ON
```

### Strategy 2: Vendored headers (for header-only libraries)

Drop headers into `third_party/<name>/` and they're automatically available if the `stb` pattern in `ThirdParty.cmake` is followed:

```bash
mkdir -p third_party/stb
curl -o third_party/stb/stb_image.h \
  https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
```

For a new vendored library, add a similar block:

```cmake
if(EXISTS ${CMAKE_SOURCE_DIR}/third_party/mylib)
    add_library(mylib INTERFACE)
    target_include_directories(mylib INTERFACE ${CMAKE_SOURCE_DIR}/third_party/mylib)
    message(STATUS "mylib headers found in third_party/mylib/")
endif()
```

### Linking to your module

After adding the dependency, link it to your library target:

```cmake
target_link_libraries(networking PRIVATE CURL::libcurl)
```

---

## Checklist

When extending the project, make sure to:

- [ ] Follow the `include/<module>/` + `src/<module>/` directory convention
- [ ] Add Doxygen comments (`@file`, `@brief`, `@param`, `@return`) to all public headers
- [ ] Use include guards matching the path: `<MODULE>_<FILE>_H`
- [ ] Return `ErrorCode` from functions that can fail (see `core/error.h`)
- [ ] Add at least one test for each new module
- [ ] Add an example demonstrating the module's usage
- [ ] Gate optional dependencies behind `option()` flags
- [ ] Update [ARCHITECTURE](ARCHITECTURE.md) if adding a new module
- [ ] Run `ctest` and `clang-tidy` before committing

---

*See [TUTORIAL](TUTORIAL.md) for a full walkthrough, [BEST_PRACTICES](BEST_PRACTICES.md) for coding standards, [SECURITY_SCANNING](SECURITY_SCANNING.md) for scan configuration.*
