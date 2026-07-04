# Examples (`examples/`)

One runnable demo per module (`<module>_demo.c` → target
`example_<module>`), registered in `examples/CMakeLists.txt`. Examples are
built by CI but not executed there — they must still be correct, clean under
sanitizers, and pass clang-tidy like all other code.

## What an example is for

An example is the module's README in executable form: a developer (or agent)
should be able to read it top to bottom and know how to use the module
correctly — including error handling. It is NOT a test (no CHECK macros) and
NOT a benchmark unless that's its explicit purpose.

## Conventions

1. **Self-contained and deterministic.** Generate input data in code
   (spiral dataset, loopback sockets) rather than reading external files.
   Seed RNGs with constants. A demo run should finish in a few seconds.
2. **Handle errors the way real code should.** Check ErrorCodes and fail
   with a message on stderr and exit code 1 — demos teach the error
   discipline by example. `if (err) { fprintf(stderr, ...); return 1; }`
   or the `DIE` macro pattern in `networking_demo.c`.
3. **Print something meaningful.** Output should show the module *worked*
   (values, timings, counts) so a human running it learns something.
4. **Free everything.** Demos are held to the same Valgrind-clean standard
   as tests — they model production code.
5. **Compose modules deliberately.** The best demos show modules working
   together (echo server = networking + hpc thread pool; ml demo = matx +
   rng + dataset + nn). Long-running servers (`example_echo_server`,
   `example_evloop_server`) run until Ctrl-C and say so in their output.

## Registering an example

```cmake
add_executable(example_foo ../examples/foo_demo.c)
target_link_libraries(example_foo PRIVATE core foo)
```

Gate examples that need optional dependencies behind the corresponding
option (see `example_cli_argtable` / `example_matmul_bench`). After adding
one, list it in the Example Targets table in `docs/ARCHITECTURE.md`.
