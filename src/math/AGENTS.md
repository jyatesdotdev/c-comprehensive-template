# math — linear algebra, RNG, statistics (`src/math/`)

Fixed-size graphics/physics types (Vec2/3/4, Mat4, Quat), dynamically sized
matrices (MatX), PCG32 randomness, and descriptive statistics. Links
`core` + `m`. **This module owns `Vec3`** — simulation/physics.h includes it.

## Two API styles, on purpose

- **Fixed-size types pass by value** and are infallible: `Vec3
  vec3_add(Vec3 a, Vec3 b)`. Small structs by value are cheap, alias-free,
  and composable. No ErrorCode, no pointers.
- **MatX is pointer + ErrorCode**: heap- or arena-backed, dimensions
  validated on every operation, aliasing rejected where the algorithm
  can't tolerate it (matmul, transpose) and allowed where it can (add).
  The `owns_data` flag makes `matx_destroy` safe for both backings.

Follow whichever style matches the new type's size and fallibility.

## Float discipline (project-wide, concentrated here)

- `f`-suffixed literals, `sinf/sqrtf/expf` variants — `-Wdouble-promotion`
  is on and will catch drift.
- Never `==` on computed floats: tests use `scalar_approx_eq` /
  `vec3_approx_eq` with explicit tolerances.
- Accumulate long sums in `double` (see stats.c, loss functions in ml);
  use Welford for variance — the naive sum-of-squares formula cancels
  catastrophically.
- Numerically risky spots get the standard fixes and a comment: softmax
  subtracts the row max; slerp falls back to nlerp near parallel; matrix
  inverse rejects |det| ~ 0.

## Conventions to preserve

- Mat4 is **column-major** (OpenGL/cglm convention): element (r,c) is
  `m[c*4 + r]`; the `M()` macro in mat.c is the only place that formula
  appears. MatX is **row-major**. Don't let the two leak into each other.
- PCG32 (`rng.c`) is deterministic by design — same seed+stream, same
  sequence, forever; tests depend on it. It is not cryptographic;
  `rng_range_u32` uses rejection sampling to stay unbiased.
- The naive/blocked matmul pair is educational — `examples/
  matmul_bench_demo.c` exists to show a real BLAS beating both by 10-100x.
  Optimize for clarity here, and route real performance needs to
  `USE_OPENBLAS`.
