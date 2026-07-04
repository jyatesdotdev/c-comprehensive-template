# containers public API (`include/containers/`)

Type-erased containers and string handling. General header rules:
`include/AGENTS.md`; implementation invariants (tombstones, growth policy,
the genericity decision): `src/containers/AGENTS.md`.

## Consumer contract

- **Element-size genericity**: `vec_init(&v, sizeof(T))`, then pass `&value`
  to push and cast `vec_at` results. For pointer payloads store the pointer
  itself (`vec_push(&v, &ptr)`). The container memcpys by value and never
  owns what pointers point at.
- **Growth invalidates interior pointers**: any pointer from `vec_at` (or
  address of a hashmap value you stashed) dies on the next growing
  operation. Reserve capacity up front if you must hold addresses
  (`containers_demo.c` shows this).
- `hashmap.h` — keys are copied (map owns them); values are `void *` you
  own. `hashmap_get` returning NULL is ambiguous if you store NULL values —
  use `hashmap_contains` to distinguish. Don't mutate the map while
  iterating with `hashmap_next`.
- `strbuf.h` — `strbuf_cstr` is always valid and NUL-terminated but is
  invalidated by further appends; `strbuf_take` transfers ownership
  (caller frees) and resets the builder.
- `str.h` — `StrView` is a *borrow*: valid only while the underlying buffer
  lives, and never NUL-terminated. Materialize with `sv_to_cstr` before
  passing to APIs expecting C strings.
- `ringbuf.h` — choose the overflow policy explicitly: `ringbuf_push`
  fails when full (`ERR_OVERFLOW`); `ringbuf_push_overwrite` drops the
  oldest. Single-threaded only — cross-thread queues live in `hpc/queue.h`.
- `hash.h` — table/integrity hashing only, never security.
