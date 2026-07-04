# Implementation Files (`src/`)

Every `.c` file here implements a public header in `include/<module>/`. The
directory structure mirrors `include/` exactly. Read the root `AGENTS.md`
first; this guide covers implementation-side rules, and each module directory
has its own `AGENTS.md` with domain-specific guidance.

## Rules for every implementation file

1. **Include your own header first.** This proves the header is
   self-contained (compiles without hidden dependencies). Then project
   headers, then system headers.
2. **Validate before work.** Every function checks pointers and ranges
   before touching them, returning `ERR_INVALID_ARG` — even for arguments a
   "reasonable caller" would never pass. Void functions return silently on
   NULL. See `src/math/matx.c` for the canonical shape.
3. **Error paths release in reverse acquisition order.** Two acceptable
   shapes: early-return with explicit cleanup at each exit (most files), or
   a single `goto cleanup` label when more than ~3 resources are in flight.
   Never mix the two in one function.
4. **State invariants the analyzers can't infer.** If a value can't be NULL
   or a loop must run at least once, either restructure so it's provable
   (preferred — see the softmax sum seeding in `src/ml/nn.c`) or add an
   early guard. Use `NOLINT(<check>)` with a justification comment only for
   intentional policy exceptions (see `src/systems/process.c`).
5. **`static` for everything internal.** File-local helpers get `static`
   linkage, a Doxygen one-liner, and no module prefix. If two files in one
   module must share a helper, declare it in a `*_internal.h` inside `src/`
   (never `include/`) — see `src/networking/socket_internal.h`.
6. **Check what the OS tells you.** Any call that can fail — allocation,
   `fclose` (it flushes!), `fseek`, `pthread_create`, socket ops — is
   checked or explicitly `(void)`-discarded with a comment saying why
   discarding is correct.

## Sizes and arithmetic

- `size_t` for sizes/indices; check `a > SIZE_MAX / b` before `a * b`.
- Widen **before** multiplying: `(size_t)w * (size_t)h`, never
  `(size_t)(w * h)` — the overflow already happened inside the cast.
- Float code uses `f`-suffixed literals and `sinf`/`sqrtf`-style functions;
  `-Wdouble-promotion` is on. Accumulate long float sums in `double`.

## Adding a file to a module

Add the source to the module's `add_library()` block in the root
`CMakeLists.txt`. Sources are listed explicitly — never use `file(GLOB)`,
which hides additions from the build system and from reviewers.
