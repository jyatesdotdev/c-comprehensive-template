# simulation public API (`include/simulation/`)

Particle physics and numerical methods. General header rules:
`include/AGENTS.md`; integrator/method notes: `src/simulation/AGENTS.md`.

## Consumer contract

- `physics.h` — `Vec3` comes from `math/vec.h` (this header includes it).
  `Particle` is transparent; for Verlet integration initialize `prev_pos`
  (usually equal to `pos` for a standing start) or the first step explodes.
  Pick one integrator per system: Euler (simple, drifts) or Verlet
  (stable for oscillatory motion); don't alternate. Masses must be
  positive — springs ignore non-positive-mass particles.
- `numerical.h` — read each method's preconditions in its Doxygen:
  Simpson needs even `n`; bisection needs `f(a)` and `f(b)` to bracket a
  root (unchecked — garbage in, garbage out); Newton needs a decent `x0`
  and bails near-zero derivatives. The RK4 functions return `ErrorCode`
  (they allocate scratch) — check it; on failure `y` holds the last
  successfully integrated state.
- Everything here is `double` precision except particle physics (`float`,
  game-style). Compare results with tolerances, not equality.
