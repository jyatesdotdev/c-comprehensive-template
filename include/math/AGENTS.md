# math public API (`include/math/`)

Linear algebra, RNG, statistics. General header rules: `include/AGENTS.md`;
float discipline and layout invariants: `src/math/AGENTS.md`.

## Consumer contract

- **This module owns `Vec3`** (`vec.h`) — include it rather than defining
  vector types. Vec2/3/4 and Quat are by-value and infallible; compose
  freely: `vec3_add(a, vec3_scale(b, t))`.
- `mat.h` — `Mat4` is **column-major** (OpenGL convention): pass `.m`
  straight to GL; transpose first for row-major consumers. Builders
  (`mat4_perspective`, `mat4_look_at`, rotations) exist so you never
  hand-fill matrices. `mat4_mul(a, b)` applies `b` first. `mat4_inverse`
  returns false for singular input — check it.
- `quat.h` — rotation quaternions must be unit length; normalize after
  accumulating many `quat_mul`s. `quat_slerp` handles the shortest arc.
- `matx.h` — **row-major**, runtime-sized, ErrorCode API. Pre-allocate
  outputs with matching dimensions; matmul/transpose reject aliased
  outputs (`ERR_INVALID_ARG`), add allows them. Heap-backed needs
  `matx_destroy`; arena-backed (`matx_init_arena`) must NOT be destroyed —
  the arena owns the memory.
- `rng.h` — seed explicitly (`rng_seed(r, seed, stream)`); same seed →
  same sequence, forever (tests depend on this). Use `rng_range_u32` for
  bounded ints (unbiased) — never `rng_next_u32() % n`. Not cryptographic.
- `stats.h` / `scalar.h` — compare computed floats with
  `scalar_approx_eq(a, b, eps)`, never `==`; `stats_variance` is the
  sample (n-1) variance.
