# systems — OS interfaces (`src/systems/`)

File I/O, memory mapping, directory walking, process control, and path
utilities. This is the module that's *allowed* to be platform-specific —
POSIX code lives behind `#ifndef _WIN32` with Windows stubs returning
`ERR_UNSUPPORTED` (see `file_io.c`). Core logic elsewhere must stay
platform-clean; anything OS-flavored belongs here.

## Per-file guidance

- `file_io.c` — the ownership rule in action: `file_read_all` returns a
  **caller-freed** buffer and its header says so. Every `fclose` on a write
  path is checked (fclose flushes; ignoring it loses write errors); error
  paths use `(void)fclose` with a comment. mmap pairs open/mmap with
  munmap/close in strict reverse order.
- `process.c` — `system`/`popen` wrappers exist deliberately; their
  cert-env33-c NOLINTs say callers own command sanitization. Never build
  command strings from untrusted input — use `process_exec` (fork/execvp,
  no shell) for anything with user-influenced arguments. The child calls
  `_exit(127)` (not `exit`) after a failed exec: no atexit handlers, no
  flushed-twice stdio.
- `path.c` — string functions are **lexical** (no filesystem access, no
  symlink resolution); only `path_exists`/`path_is_dir`/`path_mkdirs`
  touch the OS. Keep that split: callers must be able to normalize
  untrusted paths without side effects. `path_mkdirs` treats EEXIST as
  success per component but verifies the final result is a directory — a
  blocking regular file is ERR_IO, not silent success.

## C lessons this module encodes

- **errno discipline**: check it only immediately after a failing call,
  compare against specific values (EEXIST, EINTR), never store it across
  other calls.
- **EINTR**: any blocking syscall can be interrupted; either retry
  (`nanosleep` remainder loop in core/time.c) or design so interruption is
  harmless.
- **TOCTOU awareness**: `path_exists()` then `open()` is a race; prefer
  acting and handling the error. The helpers here are for convenience
  checks, not security decisions.
