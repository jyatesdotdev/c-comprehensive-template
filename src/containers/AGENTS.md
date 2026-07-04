# containers — data structures (`src/containers/`)

Type-erased generic containers (`void *` + element size), string handling,
and non-cryptographic hashing. Links `core` only.

## The genericity decision (read before adding a container)

This module uses **runtime element sizes** (`vec_init(&v, sizeof(T))`,
elements memcpy'd by value) rather than macro-generated per-type containers.
Rationale: one compiled implementation, debuggable, no macro expansion
errors — at the cost of no type checking and a memcpy per access. New
containers must follow the same style; don't introduce a second genericity
mechanism.

Consequences to document per container:
- Pointers returned by accessors (`vec_at`) are **invalidated by growth** —
  say so in the header, and in usage code reserve capacity up front when
  storing element addresses (see `containers_demo.c`).
- For pointer payloads, store the pointer itself: `vec_push(&v, &ptr)`.

## Per-file invariants

- `hashmap.c` — open addressing, linear probing, **tombstones**: deleted
  slots keep their key with `dead=1` so probe chains stay intact; they're
  only reclaimed during rehash. `used` (live + tombstones) drives the 3/4
  load-factor rehash, not `len`. Capacity stays a power of two (`hash &
  (cap-1)` replaces modulo). Keys are owned copies; values are caller-owned.
- `strbuf.c` — `data` is always NUL-terminated, even empty; every append
  goes through `ensure()` which overflow-checks before doubling. The
  two-pass `vsnprintf` sizing needs `va_copy` — the first pass consumes
  the va_list.
- `str.c` — StrView never owns memory and is never NUL-terminated; any API
  that needs a C string materializes explicitly (`sv_to_cstr`).
  `sv_split_next` uses `rest->data == NULL` as its exhaustion sentinel.
- `ringbuf.c` — `head + len` (not a tail index) avoids the full/empty
  ambiguity without wasting a slot; contrast with the SPSC queue in hpc
  which *does* waste one slot because two threads can't share `len`.
- `hash.c` — FNV-1a and CRC-32 are for tables and integrity only, never
  security. Test vectors in `test_containers.c` pin the exact outputs;
  changing constants is a breaking change.
