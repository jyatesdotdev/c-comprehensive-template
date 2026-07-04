# Examples CMakeLists.txt

add_executable(example_arena ../examples/arena_demo.c)
target_link_libraries(example_arena PRIVATE core)

add_executable(example_thread_pool ../examples/thread_pool_demo.c)
target_link_libraries(example_thread_pool PRIVATE core hpc)

add_executable(example_physics ../examples/physics_demo.c)
target_link_libraries(example_physics PRIVATE core simulation)

add_executable(example_systems ../examples/systems_demo.c)
target_link_libraries(example_systems PRIVATE core systems)

add_executable(example_hpc ../examples/hpc_demo.c)
target_link_libraries(example_hpc PRIVATE core hpc m)

add_executable(example_rendering ../examples/rendering_demo.c)
target_link_libraries(example_rendering PRIVATE core rendering_sw)

add_executable(example_simulation ../examples/simulation_demo.c)
target_link_libraries(example_simulation PRIVATE core simulation m)

add_executable(example_benchmark ../examples/benchmark_demo.c)
target_link_libraries(example_benchmark PRIVATE core hpc math m)

add_executable(example_cli ../examples/cli_demo.c)
target_link_libraries(example_cli PRIVATE cli core)

if(USE_ARGTABLE3)
    add_executable(example_cli_argtable ../examples/cli_argtable_demo.c)
    target_link_libraries(example_cli_argtable PRIVATE argtable3)
endif()

add_executable(example_networking ../examples/networking_demo.c)
target_link_libraries(example_networking PRIVATE core networking)

add_executable(example_echo_server ../examples/echo_server_demo.c)
target_link_libraries(example_echo_server PRIVATE core networking hpc)

add_executable(example_echo_client ../examples/echo_client_demo.c)
target_link_libraries(example_echo_client PRIVATE core networking)

add_executable(example_math ../examples/math_demo.c)
target_link_libraries(example_math PRIVATE core math m)

add_executable(example_ml ../examples/ml_demo.c)
target_link_libraries(example_ml PRIVATE core ml math m)

add_executable(example_matmul_bench ../examples/matmul_bench_demo.c)
target_link_libraries(example_matmul_bench PRIVATE core math m)
if(USE_OPENBLAS AND BLAS_FOUND)
    target_link_libraries(example_matmul_bench PRIVATE ${BLAS_LIBRARIES})
    target_compile_definitions(example_matmul_bench PRIVATE HAVE_CBLAS)
endif()
