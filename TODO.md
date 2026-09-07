# Hardening TODO

Audit of `c-template` (workflows: scout + correctness/security/readability/modularity
reviewers, plus parent inspection). Validation after this pass: **17/17 tests
passed** with `-DENABLE_SANITIZERS=ON` (ASan/UBSan).

Status: `[ ]` open, `[x]` done, `[~]` deferred (design / out of this pass).

---

## P0 — Correctness / memory safety

- [x] **arena_alloc overflow + align** — `src/memory/arena.c`
- [x] **leak_detect_realloc** — realloc first; failed realloc stays tracked
- [x] **cli_parse OOB** — reject `num_options` outside `0..CLI_MAX_OPTS`
- [x] **event_loop empty busy-spin + EINTR** — `poll(NULL,0,timeout)` + EINTR retry
- [x] **thread_pool submit vs destroy** — `ERR_UNSUPPORTED` after shutdown; overflow-check thread array
- [x] **file_write_all atomic** — sibling `.tmp` + `rename`
- [x] **file_read_all empty file** — dummy 1-byte buffer, not `ERR_NOMEM`
- [x] **path_normalize 64-component cap** — `ERR_OVERFLOW`
- [x] **process_capture** — overflow-check doubling; `ferror` → `ERR_IO`
- [x] **dataset_batch / CSV growth wrap** — `src/ml/dataset.c`
- [x] **fb_create size overflow + size_t indexing**
- [x] **vk_pipeline ERR_UNKNOWN** → `ERR_IO`
- [x] **HAS_NEON 32-bit ARM** — `vaddvq_f32` gated on `__aarch64__`
- [x] **numerical_bisect** — NULL/`tol<=0` + iteration cap
- [x] **physics_confine_box** — explicit `x,y,z` pointers, not `&pos.x + axis`
- [x] **physics_collide_spheres** — mass-weighted impulse (equal-mass tests unchanged)
- [x] **parallel_for malloc overflow** — fall back to inline `body(0,n)`

## P1 — Security / contracts / CI

- [x] **process_run / process_capture public docs** — shell commands; prefer `process_exec`
- [x] **process_on_sigint** — header now matches `ERR_IO`
- [x] **process_exec** — exec-fail is `ERR_OK` + 127; Windows `ERR_UNSUPPORTED`
- [x] **cli_load_config** — header matches `ERR_NOT_FOUND` + `cli_free` on error
- [x] **unix_listen unlink** — only if `lstat` is a socket; `chmod 0600`
- [x] **dir_walk DT_UNKNOWN** — `stat` fallback
- [x] **mmap size 0** — `ERR_UNSUPPORTED`
- [x] **path_mkdirs** — oversized path → `ERR_OVERFLOW`
- [x] **SOCK_CLOEXEC** — `nw_socket()` used by TCP/UDP/Unix
- [x] **CI sanitizers** — `sanitizers` job in `.github/workflows/ci.yml`
- [x] **ci.yml `permissions: contents: read`**
- [x] **security.yml Trivy** — pin `0.28.0`; `exit-code: 1` on CRITICAL,HIGH
- [x] **ENABLE_RENDERING** — find/link OpenGL and Vulkan; warn if neither found

## P1 — Tests / scaffolder / human maintainability

- [x] Extract `CHECK` into `tests/check.h`
- [x] **new_module.sh** — `CHECK` not `assert()`; stub `AGENTS.md` files
- [x] Tests for `file_io` and software renderer
- [x] Tests for arena align rejection, cli_parse overflow, event_loop empty poll, path 65 components
- [x] Header `@return` lists (`arena_init`, `pool_init`, `fb_create`, process, argparse)
- [x] `pool_init` documents silent `block_size` bump
- [x] `log.h` — DEBUG is runtime-filtered
- [x] `simd_ops.h` — SSE/NEON + scalar, not AVX kernels
- [x] `thread_pool.h` destroy/submit contract

## P2 — Docs / modularity

- [x] Root `AGENTS.md` / README: memory compiles into `core`
- [x] `docs/ARCHITECTURE.md` — `ci.yml`, stb caveat, new tests, PUBLIC math
- [x] `include/core/AGENTS.md` vs allocators
- [x] `third_party/AGENTS.md` vs stb-only special case
- [x] `perf_test.h` guard `TESTING_PERF_TEST_H`
- [x] `src/rendering/AGENTS.md` — points at `test_rendering.c`
- [x] `simulation`/`ml` `PUBLIC` link `math`

## This continuation

- [x] `tcp_listen_host` / `udp_open_host`; echo demos bind `127.0.0.1`
- [x] hashmap / event_loop growth overflow checks
- [x] `leak_detect_calloc` size overflow; `pool_free` range check
- [x] `process_on_sigint` uses `sigaction` on POSIX
- [x] Linux Release `_FORTIFY_SOURCE=2`, PIE, RELRO
- [x] `new_module.sh <existing-module> <file>` appends a source
- [x] Dependabot for GitHub Actions (weekly)
- [x] SIMD NULL/`n==0` guards; `mat4_perspective`/`ortho` identity on degenerate; Newton NULL `f`
- [x] `ENABLE_NETWORKING` (default OFF on Windows); tests/examples gated
- [x] `cmake/Install.cmake` export + BUILD/INSTALL include interfaces
- [x] Docker image ships only `example_cli` (not echo servers)
- [x] `ENABLE_HPC` (default OFF on Windows); tests/examples gated
- [x] Package config version file + `find_dependency(Threads)` when HPC is exported
- [x] `tcp_listen_host` / `udp_open_host` accept IPv6 literals (`::1`); `*` stays IPv4
- [x] MSVC `/W4` vs GCC/Clang warning set (so Windows configure is not killed by `-Wall`)
- [x] Docs/scaffolder mention BUILD/INSTALL include genex + Install.cmake targets

## Closed / out of scope

- Windows sockets, Windows CI, FILE_SET header isolation, `queue.h` split,
  `fb_*` rename, mass `vec3_*` Doxygen, clang-format align churn, allocator vtable:
  **not doing**. User does not support Windows; the rest are breaking or taste.

## Pins (Dependabot PRs #1 #2 #4 applied on main)

- [x] Unity FetchContent SHA (v2.6.0)
- [x] actions/checkout v7.0.1, upload-artifact v7.0.1, codeql upload-sarif v4.37.9
- [~] Optional FetchContent (SDL/GLFW/…) and Docker digests — unused unless `USE_*=ON`

---

## Evidence

```
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build
ctest --test-dir build --output-on-failure --timeout 120
# 17/17 passed (ASan/UBSan) after both implementation passes
```

Not committed (`COMMIT` was not granted).
