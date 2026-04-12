# Third-Party Library Integration Guide

This guide covers integrating common C libraries used in graphics, math, systems, and utility work. Each section includes CMake integration, usage patterns, and notes on cross-platform behavior.

---

## Table of Contents

1. [Windowing & Input](#windowing--input)
2. [OpenGL Loaders](#opengl-loaders)
3. [Vulkan SDK](#vulkan-sdk)
4. [Math Libraries](#math-libraries)
5. [Utility Libraries (stb, cJSON)](#utility-libraries)
6. [CMake Integration Patterns](#cmake-integration-patterns)

---

## Windowing & Input

### SDL2

SDL2 provides cross-platform windowing, input, audio, and 2D rendering. It's the most comprehensive option for multimedia applications.

**Install:**
```bash
# macOS
brew install sdl2

# Ubuntu/Debian
sudo apt install libsdl2-dev

# Windows (vcpkg)
vcpkg install sdl2
```

**CMake (system-installed):**
```cmake
find_package(SDL2 REQUIRED)
target_link_libraries(myapp PRIVATE SDL2::SDL2 SDL2::SDL2main)
```

**CMake (FetchContent):**
```cmake
include(FetchContent)
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.30.0
    GIT_SHALLOW    TRUE
)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL2)
target_link_libraries(myapp PRIVATE SDL2::SDL2-static SDL2::SDL2main)
```

**Minimal usage:**
```c
#include <SDL.h>

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *win = SDL_CreateWindow("Demo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_SHOWN);

    SDL_Event e;
    int running = 1;
    while (running) {
        while (SDL_PollEvent(&e))
            if (e.type == SDL_QUIT) running = 0;
    }

    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
```

### GLFW

GLFW is a lightweight library focused on OpenGL/Vulkan context creation and window management. Preferred for pure graphics applications.

**Install:**
```bash
# macOS
brew install glfw

# Ubuntu/Debian
sudo apt install libglfw3-dev

# Windows (vcpkg)
vcpkg install glfw3
```

**CMake (system-installed):**
```cmake
find_package(glfw3 3.3 REQUIRED)
target_link_libraries(myapp PRIVATE glfw)
```

**CMake (FetchContent):**
```cmake
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)
target_link_libraries(myapp PRIVATE glfw)
```

**Minimal usage (OpenGL context):**
```c
#include <GLFW/glfw3.h>

int main(void) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow *win = glfwCreateWindow(800, 600, "Demo", NULL, NULL);
    glfwMakeContextCurrent(win);

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        glfwSwapBuffers(win);
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
```

### SDL2 vs GLFW — When to Use Which

| Feature          | SDL2                    | GLFW                  |
|------------------|-------------------------|-----------------------|
| Audio            | Built-in                | None                  |
| 2D rendering     | SDL_Renderer            | None                  |
| Gamepad input    | Full support            | Basic support         |
| Vulkan support   | Yes                     | Yes                   |
| Binary size      | Larger                  | Minimal               |
| Best for         | Games, multimedia       | Pure graphics/compute |

---

## OpenGL Loaders

Modern OpenGL (3.3+) requires a loader to access function pointers. Two main options:

### glad (Recommended)

glad is generated per-project and compiled as a single source file — no system dependency.

**Generate at:** https://glad.dav1d.de/ (select Core profile, GL 3.3+, C language)

**Integration:**
```
third_party/
  glad/
    include/
      glad/glad.h
      KHR/khrplatform.h
    src/
      glad.c
```

**CMake:**
```cmake
add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include)
target_link_libraries(myapp PRIVATE glad glfw)
```

**Usage:**
```c
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// After creating context:
glfwMakeContextCurrent(window);
if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    fprintf(stderr, "Failed to load OpenGL\n");
    return -1;
}
```

### GLEW (Alternative)

System-installed loader. Simpler setup but adds a runtime dependency.

```cmake
find_package(GLEW REQUIRED)
target_link_libraries(myapp PRIVATE GLEW::GLEW)
```

---

## Vulkan SDK

### Installation

Download from https://vulkan.lunarg.com/sdk/home or use a package manager:

```bash
# macOS
brew install vulkan-sdk

# Ubuntu
sudo apt install vulkan-tools libvulkan-dev vulkan-validationlayers
```

### CMake Integration

```cmake
find_package(Vulkan REQUIRED)
target_link_libraries(myapp PRIVATE Vulkan::Vulkan)

# Optional: shader compilation
if(Vulkan_glslc_FOUND)
    # Compile GLSL to SPIR-V at build time
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/shaders/vert.spv
        COMMAND Vulkan::glslc ${CMAKE_SOURCE_DIR}/shaders/shader.vert
                -o ${CMAKE_BINARY_DIR}/shaders/vert.spv
        DEPENDS ${CMAKE_SOURCE_DIR}/shaders/shader.vert
    )
endif()
```

### Vulkan + GLFW Surface Creation

```c
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Check for Vulkan support
if (!glfwVulkanSupported()) {
    fprintf(stderr, "Vulkan not supported\n");
    return -1;
}

// Get required instance extensions
uint32_t count;
const char **extensions = glfwGetRequiredInstanceExtensions(&count);

// Create surface after VkInstance creation
VkSurfaceKHR surface;
glfwCreateWindowSurface(instance, window, NULL, &surface);
```

---

## Math Libraries

### cglm (Recommended for Graphics)

Full-featured GLM-like math library for C. Header-only or compiled. Optimized with SIMD.

**CMake (FetchContent):**
```cmake
FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm.git
    GIT_TAG        v0.9.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(cglm)
target_link_libraries(myapp PRIVATE cglm)
```

**Usage:**
```c
#include <cglm/cglm.h>

mat4 model, view, proj;
glm_mat4_identity(model);
glm_translate(model, (vec3){1.0f, 0.0f, 0.0f});
glm_lookat((vec3){0, 2, 5}, (vec3){0, 0, 0}, (vec3){0, 1, 0}, view);
glm_perspective(glm_rad(45.0f), 800.0f/600.0f, 0.1f, 100.0f, proj);
```

### linmath.h (Minimal, Header-Only)

Single-header math library. Good for small projects.

```bash
# Just drop the header into your project
curl -o third_party/linmath.h \
  https://raw.githubusercontent.com/datenwolf/linmath.h/master/linmath.h
```

```c
#include "linmath.h"

mat4x4 mvp;
mat4x4_identity(mvp);
mat4x4_rotate_Z(mvp, mvp, angle);
```

### HandmadeMath (Single-Header, C99)

```c
#define HANDMADE_MATH_IMPLEMENTATION
#include "HandmadeMath.h"

HMM_Vec3 pos = HMM_V3(1.0f, 2.0f, 3.0f);
HMM_Mat4 proj = HMM_Perspective_RH_NO(HMM_ToRad(45.0f), aspect, 0.1f, 100.0f);
```

### Math Library Comparison

| Library       | Style         | SIMD  | Size    | Best For              |
|---------------|---------------|-------|---------|-----------------------|
| cglm          | Function-based| Yes   | Medium  | Full graphics apps    |
| linmath.h     | Macro-based   | No    | Tiny    | Small projects        |
| HandmadeMath  | Function-based| Partial| Small  | Handmade-style code   |

---

## Utility Libraries

### stb Libraries (Single-Header)

The stb libraries are single-file public domain libraries. Include the implementation in exactly one `.c` file.

**Common stb libraries:**
- `stb_image.h` — Image loading (PNG, JPG, BMP, TGA)
- `stb_image_write.h` — Image writing
- `stb_truetype.h` — TrueType font rendering

**Setup:**
```bash
mkdir -p third_party/stb
curl -o third_party/stb/stb_image.h \
  https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
curl -o third_party/stb/stb_image_write.h \
  https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
```

**CMake:**
```cmake
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE third_party/stb)
```

**Implementation file (`src/stb_impl.c`):**
```c
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
```

**Usage:**
```c
#include "stb_image.h"

int w, h, channels;
unsigned char *img = stbi_load("texture.png", &w, &h, &channels, 4);
if (!img) { /* handle error */ }
// ... use pixel data ...
stbi_image_free(img);
```

### cJSON (JSON Parsing)

```cmake
FetchContent_Declare(
    cjson
    GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
    GIT_TAG        v1.7.18
    GIT_SHALLOW    TRUE
)
set(ENABLE_CJSON_TEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(cjson)
target_link_libraries(myapp PRIVATE cjson)
```

---

## CMake Integration Patterns

### Pattern 1: FetchContent (Preferred for Reproducibility)

```cmake
include(FetchContent)

# Declare all dependencies upfront
FetchContent_Declare(dep_name
    GIT_REPOSITORY https://github.com/org/repo.git
    GIT_TAG        v1.0.0
    GIT_SHALLOW    TRUE
)

# Suppress sub-project options
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(dep_name)
```

### Pattern 2: find_package (System Libraries)

```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIB REQUIRED IMPORTED_TARGET libname)
target_link_libraries(myapp PRIVATE PkgConfig::LIB)
```

### Pattern 3: Vendored Single-Header Libraries

```cmake
# For header-only libs in third_party/
add_library(vendor INTERFACE)
target_include_directories(vendor INTERFACE
    ${CMAKE_SOURCE_DIR}/third_party
)

# For single-header libs needing an implementation file
add_library(vendor_impl STATIC src/vendor_impl.c)
target_include_directories(vendor_impl PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party
)
```

### Pattern 4: Optional Dependencies with Graceful Fallback

```cmake
option(USE_SDL2 "Use SDL2 for windowing" OFF)

if(USE_SDL2)
    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
        target_link_libraries(myapp PRIVATE SDL2::SDL2)
        target_compile_definitions(myapp PRIVATE HAS_SDL2)
    else()
        message(WARNING "SDL2 not found — building without SDL2 support")
    endif()
endif()
```

### Recommended Project Layout

```
third_party/
  glad/              # Generated OpenGL loader
    include/
    src/
  stb/               # Single-header libraries
    stb_image.h
    stb_image_write.h
  linmath.h          # Single-header math
cmake/
  ThirdParty.cmake   # All FetchContent declarations
```

### Centralized Dependency File

Keep all `FetchContent_Declare` calls in `cmake/ThirdParty.cmake` and include it from the root `CMakeLists.txt`:

```cmake
# cmake/ThirdParty.cmake
include(FetchContent)

FetchContent_Declare(cglm ...)
FetchContent_Declare(cjson ...)
FetchContent_Declare(glfw ...)

FetchContent_MakeAvailable(cglm cjson glfw)
```

```cmake
# Root CMakeLists.txt
include(cmake/ThirdParty.cmake)
```

This keeps the root build file clean and makes dependency versions easy to audit.

---

## See Also

- [README](../README.md) — Quick start and module overview
- [ARCHITECTURE](ARCHITECTURE.md) — CMake target relationships and build flow
- [EXTENDING](EXTENDING.md) — Adding dependencies via FetchContent and vendoring
- [TOOLCHAIN](TOOLCHAIN.md) — Required tools and build prerequisites
- [CROSS_PLATFORM](CROSS_PLATFORM.md) — Platform-specific dependency handling
