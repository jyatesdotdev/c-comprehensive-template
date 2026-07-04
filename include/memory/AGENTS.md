# memory public API (`include/memory/`)

Allocators compiled into the `core` target. General header rules:
`include/AGENTS.md`; internals and the allocator-selection table:
`src/memory/AGENTS.md`.

## Consumer contract

- `arena.h` — bump allocator. `arena_alloc(a, size, align)` returns NULL
  when full (not an ErrorCode — allocation failure here is a normal
  outcome). `align` must be a power of two. Individual frees don't exist:
  `arena_reset` reclaims everything at once; pointers from before a reset
  are dead. Ideal for per-frame/per-request scratch; see `matx_init_arena`
  for the accept-an-`Arena *` pattern.
- `pool.h` — fixed-size blocks with individual `pool_free`. Blocks are at
  least `sizeof(void *)` regardless of requested size. Freeing a pointer
  that didn't come from `pool_alloc` corrupts the free list — there is no
  validation (by design; it's O(1)).
- `leak_detect.h` — educational malloc tracker enabled by defining
  `ENABLE_LEAK_DETECT` before including it; macros then shadow
  malloc/free. Prefer ASan/Valgrind (already in CI) for real leak hunting.

All three: init/destroy pairs, destroy is NULL-safe and idempotent.
