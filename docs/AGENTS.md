# Documentation (`docs/`)

Human-facing guides (Doxygen builds from here too, via `BUILD_DOCS=ON`).
The agent-facing quick reference is the root `AGENTS.md`; these files are
the depth behind it. Keep them consistent with the code — a wrong doc is
worse than no doc.

## What lives where (and what must be updated when)

| File | Content | Update when… |
|------|---------|--------------|
| `ARCHITECTURE.md` | Structure tree, dependency graph, CMake options, **Test Targets / Example Targets tables** | any module, target, or option changes |
| `EXTENDING.md` | Add-a-module/test/example/dependency recipes + checklist | the workflow or conventions change |
| `BEST_PRACTICES.md` | C standards, error handling, memory safety, API design | a convention is added or changed |
| `TUTORIAL.md` | New-developer walkthrough | build/test commands change |
| `TOOLCHAIN.md` | Required tools, install, IDE setup | tool versions/requirements change |
| `THIRD_PARTY.md` | Optional dependency integration guides | a `USE_*` option is added |
| `SECURITY_SCANNING.md` | Static analysis and sanitizer details | scan config changes |
| `CLI.md`, `OPTIMIZATION.md`, `CROSS_PLATFORM.md` | Domain guides | rarely |

The **three tables in ARCHITECTURE.md** (dependency graph bullets, Test
Targets, Example Targets) are the highest-drift-risk content in the repo:
they are hand-maintained and every new target must be added to them. The
EXTENDING.md checklist names them for this reason.

## Writing style

- Show real code from this repo, not invented snippets — every example
  should be copy-runnable against the current tree.
- Tables for enumerable facts; prose for reasoning; one `## See Also`
  block at the bottom linking sibling docs.
- When documenting a rule, include the *why* (one sentence) — rules
  without rationale get cargo-culted or dropped.
- Markdown only; keep lines under ~100 chars to match the code style.
