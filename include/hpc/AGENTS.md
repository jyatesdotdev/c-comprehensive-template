# hpc public API (`include/hpc/`)

Parallelism and SIMD. General header rules: `include/AGENTS.md`; memory
ordering and shutdown protocols: `src/hpc/AGENTS.md`.

## Consumer contract

- `thread_pool.h` — opaque handle; `thread_pool_submit` queues a task, the
  pool never frees your `arg` (the task function owns its lifecycle —
  heap-allocate per-task state and free it inside the task).
  `thread_pool_destroy` waits for queued work, then joins.
- `parallel.h` — fork/join helpers for data-parallel loops. Your `body`
  callback receives disjoint `[start, end)` ranges and must not touch
  another range's data without synchronization. These are void functions:
  on internal failure they degrade to running serially, never silently
  skipping work.
- `queue.h` — **pick by thread topology**:
  - `SpscQueue`: exactly one producer thread + one consumer thread,
    non-blocking (`push`/`pop` return false when full/empty — spin or
    `sched_yield`). Anything else is a data race.
  - `BlockingQueue`: any number of producers/consumers, blocks with
    backpressure. Shutdown protocol: `bq_close()` → producers get
    `ERR_UNSUPPORTED`, consumers drain then get `ERR_NOT_FOUND`. Loop on
    `bq_pop` until `ERR_NOT_FOUND` for a clean drain.
- `simd_ops.h` — arrays may be unaligned; any length works (scalar tail).
  Results can differ from scalar code in the last ULP — compare with
  tolerances.
