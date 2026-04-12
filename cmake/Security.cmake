# cmake/Security.cmake — Security scanning tool integration
# Provides: clang-tidy (build-time), cppcheck/valgrind/flawfinder/rats (custom targets)
#
# Usage:
#   cmake -DENABLE_CLANG_TIDY=ON -DENABLE_CPPCHECK=ON ..
#   cmake --build . --target cppcheck
#   cmake --build . --target valgrind
#   cmake --build . --target flawfinder
#   cmake --build . --target security-scan   # runs all available tools

# ── Options ─────────────────────────────────────────────────────────────────
option(ENABLE_CLANG_TIDY "Run clang-tidy during build" OFF)
option(ENABLE_CPPCHECK   "Enable cppcheck custom target" OFF)

# ── clang-tidy (build-time via CMAKE_C_CLANG_TIDY) ─────────────────────────
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy-18 clang-tidy-17 clang-tidy-16)
    if(CLANG_TIDY_EXE)
        set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_EXE}")
        message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXE}")
    else()
        message(WARNING "ENABLE_CLANG_TIDY is ON but clang-tidy not found")
    endif()
endif()

# ── cppcheck (custom target) ───────────────────────────────────────────────
find_program(CPPCHECK_EXE NAMES cppcheck)
if(CPPCHECK_EXE)
    set(_cppcheck_args
        --enable=warning,performance,portability
        --std=c17
        --error-exitcode=1
        --inline-suppr
        --suppress=missingIncludeSystem
        --suppressions-list=${CMAKE_SOURCE_DIR}/.cppcheck-suppressions
        -I ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    add_custom_target(cppcheck
        COMMAND ${CPPCHECK_EXE} ${_cppcheck_args}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running cppcheck static analysis..."
        VERBATIM
    )
    # Stricter variant for CI — also enables style checks
    add_custom_target(cppcheck-ci
        COMMAND ${CPPCHECK_EXE} ${_cppcheck_args}
            --enable=all
            --xml
            --output-file=${CMAKE_BINARY_DIR}/cppcheck-report.xml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running cppcheck (CI mode, XML output)..."
        VERBATIM
    )
    message(STATUS "cppcheck found: ${CPPCHECK_EXE}")
else()
    message(STATUS "cppcheck not found — cppcheck targets disabled")
endif()

# ── Valgrind (memory error detection via CTest memcheck) ───────────────────
find_program(VALGRIND_EXE NAMES valgrind)
if(VALGRIND_EXE)
    set(MEMORYCHECK_COMMAND ${VALGRIND_EXE})
    set(MEMORYCHECK_COMMAND_OPTIONS
        "--leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1"
    )
    include(CTest)
    add_custom_target(valgrind
        COMMAND ${CMAKE_CTEST_COMMAND}
            --test-dir ${CMAKE_BINARY_DIR}
            --force-new-ctest-process
            --test-action memcheck
        COMMENT "Running Valgrind memcheck on all tests..."
        VERBATIM
    )
    message(STATUS "Valgrind found: ${VALGRIND_EXE}")
else()
    message(STATUS "Valgrind not found — valgrind target disabled")
endif()

# ── flawfinder (source code security audit) ────────────────────────────────
find_program(FLAWFINDER_EXE NAMES flawfinder)
if(FLAWFINDER_EXE)
    add_custom_target(flawfinder
        COMMAND ${FLAWFINDER_EXE}
            --minlevel=2
            --error-level=4
            --columns
            ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include
        COMMENT "Running flawfinder security audit..."
        VERBATIM
    )
    message(STATUS "flawfinder found: ${FLAWFINDER_EXE}")
else()
    message(STATUS "flawfinder not found — flawfinder target disabled")
endif()

# ── RATS (Rough Auditing Tool for Security) ────────────────────────────────
find_program(RATS_EXE NAMES rats)
if(RATS_EXE)
    add_custom_target(rats
        COMMAND ${RATS_EXE}
            --warning 2
            --resultsonly
            ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include
        COMMENT "Running RATS security audit..."
        VERBATIM
    )
    message(STATUS "RATS found: ${RATS_EXE}")
else()
    message(STATUS "RATS not found — rats target disabled")
endif()

# ── Aggregate target ───────────────────────────────────────────────────────
add_custom_target(security-scan
    COMMENT "Running all available security scans..."
)
foreach(_tool cppcheck flawfinder rats)
    if(TARGET ${_tool})
        add_dependencies(security-scan ${_tool})
    endif()
endforeach()
