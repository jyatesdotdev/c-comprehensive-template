# ml — neural network building blocks (`src/ml/`)

Dense layers with **manual backprop** (no autograd), losses, SGD/Adam, and
dataset utilities. Links `core` + `math` + `m`. Educational by design:
each layer is a self-contained forward/backward pair you can read whole.

## The layer contract (extend by copying it)

Data layout is one sample per row (`batch x features`); weights are
`(in_dim x out_dim)`, so forward is `act(x·W + b)`. A layer:

1. Owns its parameters, gradients, and forward caches (`x_cache`,
   `z_cache`) — `dense_init`/`dense_destroy` pairs.
2. `forward` caches whatever `backward` will need; caches resize
   automatically when the batch size changes (`ensure_shape`).
3. `backward` consumes dLoss/dOut, fills `dw`/`db`, and optionally emits
   dLoss/dIn for the layer below (`dx` may be NULL at the first layer).
4. Optimizers only read `dw`/`db` — they don't know layer internals.

A new layer type (conv, embedding, …) follows exactly this shape and gets a
**numerical gradient check** in `test_ml.c` — central differences over
every parameter, per activation. That test is the module's spine; a backward
pass that isn't gradient-checked is assumed wrong. (ReLU is excluded from
checking at its kink — finite differences are unreliable there; that's the
known exception, not a loophole.)

## Numerical conventions

- Fused softmax+cross-entropy (never separate ops): subtract the row max,
  and the gradient collapses to `(p - y)/batch`. The exp-sum is seeded with
  the max term's `expf(0) == 1` so `logf(0)` is provably unreachable.
- Losses return batch-mean values and gradients already scaled by 1/n, so
  learning rates are batch-size-independent.
- Weight init matches activation: He (`sqrt(2/in)`) for ReLU, Xavier
  otherwise — via `rng_normal` with the caller's seeded Rng, never rand().

## dataset.c

CSV loading is strict (inconsistent column counts and non-numeric cells are
ERR_IO, not best-effort). `dataset_shuffle` permutes x and y in lockstep —
tests verify pairing survives. Everything is deterministic given the seed.

For real workloads: this is a teaching stack. Route serious training to a
framework, serious matmul to `USE_OPENBLAS` (see `src/math/AGENTS.md`).
