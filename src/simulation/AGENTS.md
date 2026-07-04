# simulation — physics and numerical methods (`src/simulation/`)

Particle integrators, springs, collisions, and classical numerical methods
(Simpson, bisection, Newton, RK4). Links `core` + `math` + `m`.
`physics.h` gets its `Vec3` from `math/vec.h` — never redefine it.

## physics.c

Written in terms of `vec3_*` operations; keep it that way (component-wise
`p->vel.x += ...` chains are the old style this file was migrated away
from). Guards: NULL particle arrays are no-ops, springs reject zero/negative
mass, near-zero distances early-return before dividing — every division by
a computed quantity needs a guard like these.

Integrator facts worth knowing before editing:
- Euler is simple but energy-drifts; Verlet is stable for oscillatory
  systems and derives velocity as `(pos - prev_pos) / dt` — which is why
  `dt == 0` is rejected up front.
- Collision response operates along the normal only (project velocities
  with `vec3_dot`, exchange the normal components) — the tangential
  component is untouched by design.

## numerical.c

- All fallible entry points return `ErrorCode` (`rk4` allocates scratch —
  it can fail, so it says so; that's the convention everywhere).
- Method preconditions live in the header docs and are the caller's job to
  meet, but cheap ones are validated: Simpson needs even n (returns 0.0 on
  violation — documented), bisection assumes f(a) and f(b) bracket a root,
  Newton bails when |f'(x)| ~ 0 instead of dividing.
- Doubles here (not floats): accuracy matters more than bandwidth for
  these algorithms, and the tests pin tolerances accordingly
  (`CHECK_NEAR` in `test_simulation.c`).

## C lessons this module encodes

Numerical code is validated by **physics, not plumbing**: tests integrate a
harmonic oscillator for a full period and check the analytic answer, find
roots of known polynomials, and verify conservation-ish properties
(reflection, push-apart symmetry). When you add a method, add the
known-answer test that would catch a sign error.
