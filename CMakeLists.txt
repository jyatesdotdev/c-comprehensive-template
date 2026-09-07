cmake_minimum_required(VERSION 3.20)
project(c_comprehensive_template
    VERSION 1.0.0
    DESCRIPTION "Comprehensive C template for systems, HPC, and rendering"
    LANGUAGES C
)

# ── C Standard ──────────────────────────────────────────────────────────────
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ── Build Options ───────────────────────────────────────────────────────────
option(BUILD_TESTS      "Build unit tests"              ON)
option(BUILD_EXAMPLES   "Build example programs"        ON)
option(BUILD_DOCS       "Build Doxygen documentation"   OFF)
option(ENABLE_SANITIZERS "Enable ASan/UBSan"            OFF)
option(ENABLE_RENDERING "Build rendering modules (requires OpenGL/Vulkan)" OFF)
if(WIN32)
    option(ENABLE_NETWORKING "Build POSIX networking (TCP/UDP/Unix/event loop)" OFF)
    option(ENABLE_HPC "Build pthreads HPC (thread pool, queues, SIMD)" OFF)
else()
    option(ENABLE_NETWORKING "Build POSIX networking (TCP/UDP/Unix/event loop)" ON)
    option(ENABLE_HPC "Build pthreads HPC (thread pool, queues, SIMD)" ON)
endif()

# ── POSIX feature-test macro (needed for clock_gettime, strdup, usleep, etc.
#    when compiling with -std=c17 instead of -std=gnu17) ─────────────────────
if(NOT MSVC)
    add_compile_definitions(_DEFAULT_SOURCE)
endif()

# ── Compiler Warnings ──────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wdouble-promotion
        -Wformat=2 -Wno-unused-parameter -Wnull-dereference
        -fstack-protector-strong
    )
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    add_compile_options(-Wlogical-op -Wduplicated-cond)
endif()

# Apple's linker (Xcode 15+) warns when a static library appears twice on the
# link line, which is expected here: targets link `core` explicitly and also
# receive it transitively (e.g. via hpc/simulation). Harmless — silence it.
if(APPLE)
    add_link_options(-Wl,-no_warn_duplicate_libraries)
endif()

# ── Sanitizers ──────────────────────────────────────────────────────────────
if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# ── Linux Release hardening (skip when sanitizers own the link flags) ───────
if(NOT ENABLE_SANITIZERS AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions($<$<CONFIG:Release>:_FORTIFY_SOURCE=2>)
    add_compile_options($<$<CONFIG:Release>:-fPIE>)
    add_link_options($<$<CONFIG:Release>:-pie>
                     $<$<CONFIG:Release>:-Wl,-z,relro>
                     $<$<CONFIG:Release>:-Wl,-z,now>)
endif()

# ── Coverage ────────────────────────────────────────────────────────────────
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
if(ENABLE_COVERAGE)
    add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
    add_link_options(--coverage)
endif()

# ── Security Scanning ───────────────────────────────────────────────────────
# Must be included before targets so CMAKE_C_CLANG_TIDY applies to all builds
include(cmake/Security.cmake)

# ── Platform Detection ──────────────────────────────────────────────────────
include(cmake/Platform.cmake)

# ── Third-Party Libraries (optional) ───────────────────────────────────────
include(cmake/ThirdParty.cmake)

# ── Core Library ────────────────────────────────────────────────────────────
add_library(core STATIC
    src/core/error.c
    src/core/log.c
    src/core/time.c
    src/memory/arena.c
    src/memory/pool.c
    src/memory/leak_detect.c
    src/memory/allocator.c
)
target_include_directories(core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ── CLI Library ──────────────────────────────────────────────────────────────
add_library(cli STATIC
    src/cli/argparse.c
    src/cli/output.c
)
target_include_directories(cli PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(cli PRIVATE core)

# ── Systems Library ─────────────────────────────────────────────────────────
add_library(systems STATIC
    src/systems/file_io.c
    src/systems/process.c
    src/systems/path.c
)
target_include_directories(systems PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(systems PRIVATE core)

# ── HPC Library (pthreads; off by default on Windows) ───────────────────────
if(ENABLE_HPC)
    find_package(Threads REQUIRED)
    add_library(hpc STATIC
        src/hpc/simd_ops.c
        src/hpc/thread_pool.c
        src/hpc/parallel.c
        src/hpc/spsc.c
        src/hpc/blocking_queue.c
    )
    target_include_directories(hpc PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(hpc PRIVATE core Threads::Threads)
endif()

# ── Simulation Library ──────────────────────────────────────────────────────
add_library(simulation STATIC
    src/simulation/physics.c
    src/simulation/numerical.c
)
target_include_directories(simulation PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(simulation PUBLIC math PRIVATE core m)

# ── Software Renderer (always available, no external deps) ──────────────────
add_library(rendering_sw STATIC src/rendering/software_renderer.c)
target_include_directories(rendering_sw PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(rendering_sw PRIVATE core)

# ── GL/Vulkan Rendering (optional, requires graphics libraries) ─────────────
if(ENABLE_RENDERING)
    add_library(rendering STATIC
        src/rendering/gl_pipeline.c
        src/rendering/vk_pipeline.c
    )
    target_include_directories(rendering PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
    target_link_libraries(rendering PRIVATE core)
    if(OpenGL_FOUND)
        target_link_libraries(rendering PRIVATE OpenGL::GL)
    endif()
    if(Vulkan_FOUND)
        target_link_libraries(rendering PRIVATE Vulkan::Vulkan)
    endif()
    if(NOT OpenGL_FOUND AND NOT Vulkan_FOUND)
        message(WARNING "ENABLE_RENDERING=ON but neither OpenGL nor Vulkan was found")
    endif()
endif()

# ── Networking Library (POSIX sockets; off by default on Windows) ───────
if(ENABLE_NETWORKING)
    add_library(networking STATIC
        src/networking/socket.c
        src/networking/udp.c
        src/networking/unix_socket.c
        src/networking/event_loop.c
    )
    target_include_directories(networking PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
    target_link_libraries(networking PRIVATE core)
endif()

# ── Math Library ──────────────────────────────────────────────────
add_library(math STATIC
    src/math/vec.c
    src/math/mat.c
    src/math/quat.c
    src/math/matx.c
    src/math/rng.c
    src/math/stats.c
)
target_include_directories(math PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(math PRIVATE core m)

# ── ML Library ──────────────────────────────────────────────────
add_library(ml STATIC
    src/ml/nn.c
    src/ml/dataset.c
)
target_include_directories(ml PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(ml PUBLIC math PRIVATE core m)

# ── Containers Library ──────────────────────────────────────────────────
add_library(containers STATIC
    src/containers/hash.c
    src/containers/vec.c
    src/containers/hashmap.c
    src/containers/strbuf.c
    src/containers/ringbuf.c
    src/containers/str.c
)
target_include_directories(containers PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(containers PRIVATE core)

# ── Testing Frameworks ──────────────────────────────────────────────────────
if(BUILD_TESTS)
    enable_testing()
    include(cmake/Testing.cmake)
    add_subdirectory(tests)
endif()

# ── Examples ────────────────────────────────────────────────────────────────
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ── Documentation ───────────────────────────────────────────────────────────
if(BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    add_subdirectory(docs)
endif()

include(cmake/Install.cmake)
