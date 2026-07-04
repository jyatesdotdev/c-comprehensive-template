/**
 * @file nn.h
 * @brief Minimal neural network building blocks: dense layers with manual
 *        backprop, activations, losses, and SGD/Adam optimizers.
 *
 * Educational, dependency-free implementation on top of math/matx.h. Each
 * layer knows its own forward and backward pass — there is no autograd.
 * Data layout: one sample per row (batch x features), weights are
 * (in_dim x out_dim), so forward is `y = act(x·W + b)`.
 *
 * For production-scale training use a real framework; for BLAS-backed
 * matmul benchmarks see examples/matmul_bench_demo.c (USE_OPENBLAS).
 */
#ifndef ML_NN_H
#define ML_NN_H

#include "core/error.h"
#include "math/matx.h"
#include "math/rng.h"

/** @brief Activation applied after a dense layer's affine transform. */
typedef enum {
    ACT_LINEAR,  /**< Identity — use for output layers feeding a loss. */
    ACT_RELU,    /**< max(0, x) — He-initialized. */
    ACT_SIGMOID, /**< 1/(1+e^-x) — Xavier-initialized. */
    ACT_TANH,    /**< tanh(x) — Xavier-initialized. */
} Activation;

/**
 * @brief Fully connected layer with cached forward state for backprop.
 *
 * All matrices are owned by the layer (dense_init/dense_destroy). Caches
 * are sized on first forward and resized automatically if the batch changes.
 */
typedef struct DenseLayer {
    size_t     in_dim;  /**< Input feature count. */
    size_t     out_dim; /**< Output feature count. */
    Activation act;     /**< Activation function. */
    MatX       w;       /**< Weights (in_dim x out_dim). */
    MatX       b;       /**< Bias (1 x out_dim). */
    MatX       dw;      /**< Weight gradient, filled by dense_backward. */
    MatX       db;      /**< Bias gradient, filled by dense_backward. */
    MatX       x_cache; /**< Last forward input (batch x in_dim). */
    MatX       z_cache; /**< Last pre-activation (batch x out_dim). */
} DenseLayer;

/**
 * @brief Initialize a dense layer with activation-appropriate random weights
 *        (He for ReLU, Xavier otherwise) and zero bias.
 * @param l       Layer to initialize.
 * @param in_dim  Input feature count (> 0).
 * @param out_dim Output feature count (> 0).
 * @param act     Activation function.
 * @param rng     Seeded generator for weight init.
 * @return ERR_OK, ERR_INVALID_ARG, or ERR_NOMEM.
 */
ErrorCode dense_init(DenseLayer *l, size_t in_dim, size_t out_dim, Activation act, Rng *rng);

/** @brief Free all layer storage. Safe on NULL or already-destroyed layers. */
void dense_destroy(DenseLayer *l);

/**
 * @brief Forward pass: out = act(x·W + b). Caches x and the pre-activation
 *        for the subsequent backward pass.
 * @param l   Layer.
 * @param x   Input batch (batch x in_dim).
 * @param out Pre-allocated output (batch x out_dim).
 * @return ERR_OK, ERR_INVALID_ARG on dimension mismatch, ERR_NOMEM.
 */
ErrorCode dense_forward(DenseLayer *l, const MatX *x, MatX *out);

/**
 * @brief Backward pass. Consumes the loss gradient w.r.t. this layer's
 *        output and fills l->dw and l->db.
 * @param l    Layer (must have run dense_forward on this batch).
 * @param dout Gradient w.r.t. layer output (batch x out_dim).
 * @param dx   Pre-allocated gradient w.r.t. input (batch x in_dim), or NULL
 *             if not needed (first layer).
 * @return ERR_OK, ERR_INVALID_ARG, or ERR_NOMEM.
 */
ErrorCode dense_backward(DenseLayer *l, const MatX *dout, MatX *dx);

/**
 * @brief Vanilla SGD update: w -= lr * dw, b -= lr * db.
 * @param l  Layer with gradients from dense_backward.
 * @param lr Learning rate (> 0).
 */
ErrorCode dense_sgd_step(DenseLayer *l, float lr);

/** @brief Per-layer Adam optimizer state (first/second moment estimates). */
typedef struct AdamState {
    MatX mw, vw; /**< Moments for weights. */
    MatX mb, vb; /**< Moments for bias. */
    int  t;      /**< Step count (for bias correction). */
} AdamState;

/** @brief Allocate zeroed Adam moments matching a layer's shapes. */
ErrorCode adam_init(AdamState *s, const DenseLayer *l);

/** @brief Free Adam state. Safe on NULL. */
void adam_destroy(AdamState *s);

/**
 * @brief Adam update with standard hyperparameters (beta1=0.9, beta2=0.999,
 *        eps=1e-8) using the layer's current dw/db.
 * @param s  Optimizer state for this layer.
 * @param l  Layer with gradients from dense_backward.
 * @param lr Learning rate (> 0).
 */
ErrorCode adam_step(AdamState *s, DenseLayer *l, float lr);

/* ── Losses (each returns the batch-mean loss and the gradient) ─────────── */

/**
 * @brief Mean squared error: mean over all elements of (pred - target)^2.
 * @param pred     Predictions (batch x dims).
 * @param target   Targets (batch x dims).
 * @param out_loss Receives the scalar loss (may be NULL).
 * @param dpred    Pre-allocated gradient w.r.t. pred (may be NULL).
 */
ErrorCode loss_mse(const MatX *pred, const MatX *target, float *out_loss, MatX *dpred);

/**
 * @brief Fused softmax + cross-entropy over rows of logits.
 *
 * Numerically stable (max-subtraction). The gradient of the fused op is
 * simply (softmax(logits) - onehot) / batch — feed logits from an
 * ACT_LINEAR output layer.
 * @param logits   Raw scores (batch x classes).
 * @param onehot   One-hot targets (batch x classes).
 * @param out_loss Receives the mean cross-entropy (may be NULL).
 * @param dlogits  Pre-allocated gradient w.r.t. logits (may be NULL).
 */
ErrorCode loss_softmax_xent(const MatX *logits, const MatX *onehot, float *out_loss, MatX *dlogits);

#endif /* ML_NN_H */
