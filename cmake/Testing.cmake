# cmake/Testing.cmake — Testing framework integration
# Provides: Unity (FetchContent), cmocka (system), perf_test (header-only)

include(FetchContent)

# ── Unity Test Framework (always available via FetchContent) ────────────────
option(USE_UNITY "Fetch and build Unity test framework" ON)

if(USE_UNITY)
    FetchContent_Declare(
        unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG        v2.6.0
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(unity)

    # Create a library target for easy linking
    if(NOT TARGET unity::framework)
        add_library(unity_framework STATIC
            ${unity_SOURCE_DIR}/src/unity.c
        )
        target_include_directories(unity_framework PUBLIC
            ${unity_SOURCE_DIR}/src
        )
        add_library(unity::framework ALIAS unity_framework)
    endif()
endif()

# ── cmocka (optional, system-installed) ─────────────────────────────────────
option(USE_CMOCKA "Use cmocka test framework (must be installed)" OFF)

if(USE_CMOCKA)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(CMOCKA IMPORTED_TARGET cmocka)
    endif()
    if(NOT CMOCKA_FOUND)
        find_library(CMOCKA_LIB cmocka)
        find_path(CMOCKA_INC cmocka.h)
        if(CMOCKA_LIB AND CMOCKA_INC)
            add_library(cmocka::cmocka UNKNOWN IMPORTED)
            set_target_properties(cmocka::cmocka PROPERTIES
                IMPORTED_LOCATION "${CMOCKA_LIB}"
                INTERFACE_INCLUDE_DIRECTORIES "${CMOCKA_INC}"
            )
            set(CMOCKA_FOUND TRUE)
        else()
            message(WARNING "cmocka not found — cmocka tests will be skipped")
        endif()
    else()
        add_library(cmocka::cmocka ALIAS PkgConfig::CMOCKA)
    endif()
endif()

# ── Performance Test Header (always available) ──────────────────────────────
# Header-only: include/testing/perf_test.h — no library target needed.
