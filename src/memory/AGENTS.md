# memory — allocators (`src/memory/`)

Arena (bump) and pool (free-list) allocators plus a leak tracker. These
compile into the `core` library target, so like core they depend on nothing.

## When to use which allocator (the design lesson)

| Allocator | Lifetime shape | Free cost | Use for |
|-----------|----------------|-----------|---------|
| `arena`   | many allocs, one release point | O(1) reset for everything | per-frame / per-request scratch |
| `pool`    | fixed-size objects, individual free | O(1) push to free list | nodes, particles, connections |
| `malloc`  | unpredictable sizes and lifetimes | varies | everything else |

Downstream modules accept an `Arena *` where bulk-then-discard fits (see
`matx_init_arena`) — prefer extending that pattern over sprinkling malloc.

## Invariants to preserve

- `arena_alloc` alignment must be a power of two; the bump computation
  `(pos + align - 1) & ~(align - 1)` depends on it. Alignment-then-bounds
  order matters: check `aligned + size > cap` *after* aligning.
- The pool free list stores the next-pointer **inside** freed blocks —
  that's why `pool_init` clamps `block_size` to `sizeof(void *)` minimum
  and why reading a freed block is corruption, not just staleness.
- Destroy functions null out pointers and zero sizes (use-after-destroy
  becomes a clean crash or a caught NULL, not silent reuse).
- Size math is overflow-checked before multiplying (`ERR_OVERFLOW`).

## leak_detect

A teaching tool: macro-wrapped malloc/free tracked in a linked list. The
`#undef ENABLE_LEAK_DETECT` dance at the top of `leak_detect.c` prevents the
macros from wrapping the tracker's own allocations — understand it before
touching the include order. Real projects should prefer ASan/Valgrind
(both run in CI); this exists to show how interposition works.
