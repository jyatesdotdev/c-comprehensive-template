# Vendored Dependencies (`third_party/`)

Header-only libraries copied into the repo (the `stb` pattern —
`ThirdParty.cmake` exposes any `third_party/<name>/` directory it finds as
an INTERFACE target). Source-built dependencies do NOT go here; they're
fetched via `USE_*` options in `cmake/ThirdParty.cmake` (see
`cmake/AGENTS.md` and `docs/THIRD_PARTY.md`).

## Rules for vendored code

- **Never edit vendored files.** Fixes go upstream; if you must patch,
  record the exact change and upstream version in a `PATCHES.md` next to
  the file so updates don't silently revert it.
- Record provenance: library name, version/commit, upstream URL, and
  license. Keep the upstream license file alongside the code.
- Vendored code is exempt from our gates by configuration — clang-format
  and clang-tidy exclude `third_party/` (`:!third_party/*` in the format
  check, suppressions in `.cppcheck-suppressions`). Don't "clean it up"
  to our style; diffability against upstream is worth more.
- Prefer vendoring only single-header libraries with permissive licenses.
  Anything that needs compilation or has transitive dependencies belongs
  in `ThirdParty.cmake` as an opt-in `USE_*` option instead.
