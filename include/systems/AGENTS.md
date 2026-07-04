# systems public API (`include/systems/`)

OS interfaces: files, processes, paths. General header rules:
`include/AGENTS.md`; POSIX/errno discipline: `src/systems/AGENTS.md`.

## Consumer contract

- `file_io.h` — `file_read_all` returns a **caller-freed** buffer;
  `file_write_all`'s ERR_OK means the data reached the OS (fclose checked).
  `MappedFile` pairs `file_mmap_read`/`file_mmap_rw` with `file_munmap`.
  On Windows the mmap/dir functions return `ERR_UNSUPPORTED` — handle it.
- `process.h` — `process_run`/`process_capture` go through the shell:
  **never** feed them strings built from untrusted input; use
  `process_exec` (argv array, no shell) for anything user-influenced.
  `process_capture` returns a caller-freed, NUL-terminated buffer.
- `path.h` — `path_join`/`path_normalize`/`path_basename`/`path_dirname`/
  `path_ext` are purely lexical (no filesystem access, no symlinks
  resolved); only `path_exists`/`path_is_dir`/`path_mkdirs` touch the OS.
  Don't use `path_exists` as a security check (TOCTOU) — act and handle
  the error instead. `path_basename`/`path_ext` return pointers **into
  your input string**, not copies.
