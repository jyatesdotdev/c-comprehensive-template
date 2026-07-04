# ml public API (`include/ml/`)

Neural-network building blocks. General header rules: `include/AGENTS.md`;
the layer contract and gradient-check requirement: `src/ml/AGENTS.md`.

## Consumer contract

- Data layout is **one sample per row** (`batch x features`) everywhere.
- `nn.h` — the training loop shape (see `examples/ml_demo.c`):
  1. `dense_forward` each layer (caller pre-allocates activations),
  2. a loss function produces the scalar and the output gradient,
  3. `dense_backward` in reverse layer order (pass NULL `dx` at the first
     layer),
  4. `adam_step`/`dense_sgd_step` per layer.
  Backward requires the immediately preceding forward on the same batch.
  For classification, end with an `ACT_LINEAR` layer and feed the raw
  logits to `loss_softmax_xent` — do not apply your own softmax first
  (the fused op is the numerically stable one).
- `AdamState` is per-layer, allocated against that layer's shapes
  (`adam_init(&s, &layer)`), destroyed separately.
- `dataset.h` — `dataset_load_csv` is strict (ragged rows / non-numeric
  cells are `ERR_IO`); shuffle x and y **together** via `dataset_shuffle`;
  labels for `dataset_one_hot` are a `(n x 1)` matrix of class indices.
  Everything is deterministic given your `Rng` seed.

This is a teaching stack: correct, minimal, CPU-only. Route production
training to a real framework and heavy matmul to `USE_OPENBLAS`.
