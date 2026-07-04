/**
 * @file dataset.h
 * @brief Dataset utilities: CSV loading, shuffling, batching, normalization.
 *
 * Convention: one sample per MatX row. Feature matrix x and label matrix y
 * are kept separate and shuffled/batched in lockstep.
 */
#ifndef ML_DATASET_H
#define ML_DATASET_H

#include "core/error.h"
#include "math/matx.h"
#include "math/rng.h"

/**
 * @brief Load a numeric CSV file into a matrix (all cells parsed as float).
 * @param path       CSV file path.
 * @param has_header Nonzero to skip the first line.
 * @param out        Receives the loaded matrix (free with matx_destroy).
 * @return ERR_OK, ERR_INVALID_ARG, ERR_NOT_FOUND if the file can't be opened,
 *         ERR_IO on malformed rows (inconsistent column counts), ERR_NOMEM.
 * @note Lines longer than 4095 bytes are rejected with ERR_UNSUPPORTED.
 */
ErrorCode dataset_load_csv(const char *path, int has_header, MatX *out);

/**
 * @brief Fisher-Yates shuffle of x's rows, with y's rows permuted identically.
 * @param x   Feature matrix (n x d).
 * @param y   Label matrix (n x k), or NULL to shuffle x alone.
 * @param rng Seeded generator (same seed → same permutation).
 */
ErrorCode dataset_shuffle(MatX *x, MatX *y, Rng *rng);

/**
 * @brief Copy rows [start, start+count) of src into dst (a mini-batch).
 * @param src   Source matrix.
 * @param start First row index.
 * @param count Row count (start + count must be <= src->rows).
 * @param dst   Pre-allocated destination (count x src->cols).
 */
ErrorCode dataset_batch(const MatX *src, size_t start, size_t count, MatX *dst);

/**
 * @brief Standardize each column in place: (value - column mean) / column std.
 *        Columns with ~zero variance are left centered but not scaled.
 * @param x Feature matrix to normalize (n x d, n >= 2).
 */
ErrorCode dataset_normalize_zscore(MatX *x);

/**
 * @brief Expand integer class labels into one-hot rows.
 * @param labels  Label matrix (n x 1) holding values in [0, classes).
 * @param classes Number of classes.
 * @param out     Pre-allocated result (n x classes); zeroed and filled.
 * @return ERR_OK, or ERR_INVALID_ARG (including out-of-range labels).
 */
ErrorCode dataset_one_hot(const MatX *labels, size_t classes, MatX *out);

/**
 * @brief Fraction of rows where pred's argmax equals target's argmax.
 * @param pred    Predictions (n x classes), e.g. logits or probabilities.
 * @param target  One-hot targets (n x classes).
 * @param out_acc Receives accuracy in [0, 1].
 */
ErrorCode dataset_accuracy(const MatX *pred, const MatX *target, float *out_acc);

#endif /* ML_DATASET_H */
