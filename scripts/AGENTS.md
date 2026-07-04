# Scripts (`scripts/`)

Deterministic automation for repetitive multi-file edits. Prefer extending a
script over documenting a manual procedure — scripts don't skip steps.

## `new_module.sh <module> [file]`

The canonical way to add a library module. It creates the header, source,
test, and example (all convention-compliant), and registers every CMake
target. Names must be `lowercase_snake`. Do NOT register targets manually
after running it — they're already registered. What it can't do for you:
implement the real API, extend the test beyond the placeholder, and update
`docs/ARCHITECTURE.md` (it prints a checklist of these).

## Shell script standards

- `#!/usr/bin/env bash` + `set -euo pipefail` — fail fast, no silent errors.
- A header comment with usage, arguments, and what the script creates or
  modifies. A `die()` helper for argument validation with clear messages.
- Validate inputs before touching anything; refuse to overwrite existing
  work (`new_module.sh` rejects existing modules by checking both the
  directory and the CMake target).
- Anchor file edits on stable markers (e.g. the `# ── Testing Frameworks`
  section comment), and fail loudly if the anchor is missing rather than
  writing to the wrong place.
- Must run on both macOS (BSD userland, bash 3.2 — no `${var^^}`) and
  Linux. Use `tr` for case conversion, `awk` for structured edits.

Generated C code must pass the same gates as handwritten code (clang-format,
clang-tidy) — keep the heredoc templates compliant by construction, and if
you change the templates, scaffold a scratch module and build it to prove
the output still compiles and passes.
