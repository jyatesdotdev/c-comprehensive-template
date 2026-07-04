# Tests CMakeLists.txt
# Supports three test backends: minimal (built-in), Unity, cmocka.

# ── Minimal tests (always built) ───────────────────────────────────────────
add_executable(test_arena test_arena.c)
target_link_libraries(test_arena PRIVATE core)
add_test(NAME test_arena COMMAND test_arena)

add_executable(test_pool test_pool.c)
target_link_libraries(test_pool PRIVATE core)
add_test(NAME test_pool COMMAND test_pool)

add_executable(test_leak_detect test_leak_detect.c)
target_link_libraries(test_leak_detect PRIVATE core)
add_test(NAME test_leak_detect COMMAND test_leak_detect)

add_executable(test_hpc test_hpc.c)
target_link_libraries(test_hpc PRIVATE core hpc m)
add_test(NAME test_hpc COMMAND test_hpc)

add_executable(test_simulation test_simulation.c)
target_link_libraries(test_simulation PRIVATE core simulation m)
add_test(NAME test_simulation COMMAND test_simulation)

# ── Unity tests (fetched automatically when USE_UNITY=ON) ──────────────────
if(TARGET unity::framework)
    add_executable(test_memory_unity test_memory_unity.c)
    target_link_libraries(test_memory_unity PRIVATE unity::framework core)
    add_test(NAME test_memory_unity COMMAND test_memory_unity)

    add_executable(test_cli test_cli.c)
    target_link_libraries(test_cli PRIVATE unity::framework cli core)
    add_test(NAME test_cli COMMAND test_cli)
endif()

# ── cmocka tests (requires system install, USE_CMOCKA=ON) ──────────────────
if(USE_CMOCKA AND CMOCKA_FOUND)
    add_executable(test_memory_cmocka test_memory_cmocka.c)
    target_link_libraries(test_memory_cmocka PRIVATE cmocka::cmocka core)
    add_test(NAME test_memory_cmocka COMMAND test_memory_cmocka)
endif()

# ── Performance tests (always built, uses header-only perf_test.h) ──────────
add_executable(test_perf_memory test_perf_memory.c)
target_link_libraries(test_perf_memory PRIVATE core)
add_test(NAME test_perf_memory COMMAND test_perf_memory)

add_executable(test_networking test_networking.c)
target_link_libraries(test_networking PRIVATE core networking)
add_test(NAME test_networking COMMAND test_networking)

add_executable(test_math test_math.c)
target_link_libraries(test_math PRIVATE core math m)
add_test(NAME test_math COMMAND test_math)

add_executable(test_ml test_ml.c)
target_link_libraries(test_ml PRIVATE core ml math m)
add_test(NAME test_ml COMMAND test_ml)
