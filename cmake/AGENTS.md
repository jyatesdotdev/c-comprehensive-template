# Build System (`cmake/`)

The root `CMakeLists.txt` orchestrates; these files handle cross-cutting
concerns. Inclusion order matters: `Security.cmake` first (so
`CMAKE_C_CLANG_TIDY` applies to every target defined after it), then
`Platform.cmake`, then `ThirdParty.cmake`, then targets.

| File | Responsibility |
|------|----------------|
| `Security.cmake` | clang-tidy inline analysis; cppcheck/valgrind/flawfinder/rats custom targets |
| `Platform.cmake` | OS + SIMD detection (`HAS_SSE42`/`HAS_NEON` compile defs), GL/Vulkan linkage |
| `Testing.cmake`  | Unity via FetchContent, optional system cmocka |
| `ThirdParty.cmake` | All optional dependencies (`USE_*` options) |

## Patterns to follow

**Library module block** (root CMakeLists, before `# ── Testing Frameworks`):
```cmake
add_library(foo STATIC
    src/foo/bar.c          # list sources explicitly — never file(GLOB)
)
target_include_directories(foo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(foo PRIVATE core)   # + m, Threads::Threads, math ...
```
`scripts/new_module.sh` generates this; extend the block rather than
recreating it.

**Optional dependency block** (`ThirdParty.cmake`):
- Always gate behind `option(USE_<NAME> ... OFF)` — opt-in, never default-on.
- Try `find_package()` first; fall back to FetchContent with a pinned
  `GIT_TAG` and `GIT_SHALLOW TRUE`. Heavy libraries (OpenBLAS) are
  find-only — don't build the world from source.
- Disable the dependency's own tests/examples via cache variables.

**Third-party code is exempt from our lint rules.** Anything fetched into
`build/_deps` must have its clang-tidy cleared:
`set_target_properties(<target> PROPERTIES C_CLANG_TIDY "")` — see the
Unity handling in `Testing.cmake`. Forgetting this breaks the clang-tidy CI
job on code we don't own.

## Hard-won constraints (don't rediscover these)

- Compiler flags in the root file include GCC-only warnings guarded by
  `CMAKE_C_COMPILER_ID MATCHES "GNU"`; clang-tidy tolerates them because
  `.clang-tidy` disables `clang-diagnostic-unknown-warning-option`.
- Release builds define `NDEBUG` — this is why tests must use CHECK, not
  assert (see `tests/AGENTS.md`).
- The Apple linker duplicate-library warning is silenced deliberately
  (static libs appear twice via transitive PRIVATE deps — harmless).
- Coverage capture ignores gcov "negative" errors: gcov counters aren't
  atomic, so multithreaded tests produce transiently negative counts.

After any build-system change, verify **both** configurations:
Debug + `-DENABLE_SANITIZERS=ON`, and Release.
