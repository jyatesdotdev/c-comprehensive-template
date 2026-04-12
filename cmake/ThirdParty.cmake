# cmake/ThirdParty.cmake — Optional third-party library integration
# Include from root CMakeLists.txt when external deps are needed.
#
# Usage:
#   cmake -DUSE_SDL2=ON -DUSE_GLFW=ON -DUSE_CGLM=ON ..

include(FetchContent)

# ── SDL2 (windowing, input, audio) ──────────────────────────────────────────
option(USE_SDL2 "Use SDL2 for windowing/input" OFF)
if(USE_SDL2)
    find_package(SDL2 QUIET)
    if(NOT SDL2_FOUND)
        FetchContent_Declare(SDL2
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG        release-2.30.0
            GIT_SHALLOW    TRUE
        )
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON  CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(SDL2)
    endif()
    message(STATUS "SDL2 enabled")
endif()

# ── GLFW (lightweight OpenGL/Vulkan windowing) ──────────────────────────────
option(USE_GLFW "Use GLFW for windowing" OFF)
if(USE_GLFW)
    find_package(glfw3 3.3 QUIET)
    if(NOT glfw3_FOUND)
        FetchContent_Declare(glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG        3.4
            GIT_SHALLOW    TRUE
        )
        set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(glfw)
    endif()
    message(STATUS "GLFW enabled")
endif()

# ── cglm (SIMD-optimized graphics math) ────────────────────────────────────
option(USE_CGLM "Use cglm math library" OFF)
if(USE_CGLM)
    FetchContent_Declare(cglm
        GIT_REPOSITORY https://github.com/recp/cglm.git
        GIT_TAG        v0.9.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(cglm)
    message(STATUS "cglm enabled")
endif()

# ── cJSON (JSON parsing) ───────────────────────────────────────────────────
option(USE_CJSON "Use cJSON library" OFF)
if(USE_CJSON)
    FetchContent_Declare(cjson
        GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
        GIT_TAG        v1.7.18
        GIT_SHALLOW    TRUE
    )
    set(ENABLE_CJSON_TEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(cjson)
    message(STATUS "cJSON enabled")
endif()

# ── argtable3 (structured CLI argument parsing) ─────────────────────────────
option(USE_ARGTABLE3 "Use argtable3 for structured CLI parsing" OFF)
if(USE_ARGTABLE3)
    FetchContent_Declare(argtable3
        GIT_REPOSITORY https://github.com/argtable/argtable3.git
        GIT_TAG        v3.3.1
        GIT_SHALLOW    TRUE
    )
    set(ARGTABLE3_ENABLE_TESTS    OFF CACHE BOOL "" FORCE)
    set(ARGTABLE3_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(argtable3)
    message(STATUS "argtable3 enabled")
endif()

# ── stb (single-header utilities — vendored in third_party/stb/) ───────────
if(EXISTS ${CMAKE_SOURCE_DIR}/third_party/stb)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE ${CMAKE_SOURCE_DIR}/third_party/stb)
    message(STATUS "stb headers found in third_party/stb/")
endif()
