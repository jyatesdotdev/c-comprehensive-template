# hpc — parallelism and SIMD (`src/hpc/`)

SIMD kernels, a pthreads thread pool, fork/join helpers, and cross-thread
queues. Links `core` + `Threads::Threads`. This is the module where memory
ordering and data races live — change nothing here casually, and run the
threaded tests under sanitizers after any edit.

## Concurrency rules (project-wide, enforced here)

- No shared mutable state without synchronization. Mutex + condvar for
  complex state (thread_pool, BlockingQueue), C11 atomics for the SPSC
  fast path — never "it's probably fine" plain loads.
- Condition variables are **always** waited on in a `while` loop re-checking
  the predicate (spurious wakeups are real), and signaled while holding the
  mutex.
- Every `pthread_create` is checked. A failed create means an uninitialized
  handle — joining it is UB. The two recovery patterns: tear down and
  return ERR_IO (thread_pool_create), or run the work inline and skip that
  join (parallel_for).
- Clean shutdown is: set flag under lock → broadcast → join all → drain →
  destroy. In that order.

## Per-file invariants

- `queue.c` (SPSC) — the acquire/release pairs are load-bearing and each
  has a comment saying what it pairs with. One slot is deliberately wasted
  (`slots = capacity + 1`) so `head == tail` means empty without a shared
  length counter. `head`/`tail` sit on separate cache lines
  (`_Alignas(64)`) to prevent false sharing. Exactly one producer thread
  and one consumer thread — that contract is what makes lock-freedom sound.
- `queue.c` (BlockingQueue) — `closed` semantics: push → ERR_UNSUPPORTED,
  pop drains then ERR_NOT_FOUND. This is the shutdown protocol demos and
  tests rely on.
- `thread_pool.c` — workers exit only when `shutdown && queue empty`;
  tasks own their `arg` lifecycle (the pool frees the Task node, never the
  arg).
- `simd_ops.c` — every kernel has the same shape: vectorized main loop by
  4, scalar tail loop for the remainder, guarded by `HAS_NEON`/`HAS_SSE42`
  from Platform.cmake, with an unconditional scalar fallback. New kernels
  must keep all three tiers and get an odd-length test.

## C lessons this module encodes

Prefer message passing (queues) over shared state; keep critical sections
minimal (copy out under the lock, work outside it); benchmark before
believing lock-free beats a mutex for your workload.
