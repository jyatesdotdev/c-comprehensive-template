/**
 * @file queue.h
 * @brief Umbrella header for cross-thread queues (SPSC + blocking MPMC).
 *
 * Prefer the specific header for new code:
 *   - hpc/spsc.h           — lock-free SPSC ring (C11 atomics)
 *   - hpc/blocking_queue.h — blocking MPMC (pthread mutex + condvars)
 *
 * This header includes both so existing `#include "hpc/queue.h"` keeps working.
 */
#ifndef HPC_QUEUE_H
#define HPC_QUEUE_H

#include "hpc/blocking_queue.h"
#include "hpc/spsc.h"

#endif /* HPC_QUEUE_H */
