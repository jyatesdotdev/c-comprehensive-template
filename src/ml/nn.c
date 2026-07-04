/**
 * @file nn.c
 * @brief Dense layers with manual backprop, losses, and optimizers.
 */
#include "ml/nn.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define ADAM_BETA1 0.9f
#define ADAM_BETA2 0.999f
#define ADAM_EPS   1e-8f

/* ── Activations (element-wise, used via matx_map) ──────────────────────── */

static float act_relu(float x) {
    return x > 0.0f ? x : 0.0f;
}

static float act_relu_grad(float z) {
    return z > 0.0f ? 1.0f : 0.0f;
}

static float act_sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

static float act_sigmoid_grad(float z) {
    float s = act_sigmoid(z);
    return s * (1.0f - s);
}

static float act_tanh(float x) {
    return tanhf(x);
}

static float act_tanh_grad(float z) {
    float t = tanhf(z);
    return 1.0f - t * t;
}

static float act_identity(float x) {
    return x;
}

static float act_identity_grad(float z) {
    (void)z;
    return 1.0f;
}

static float (*act_fn(Activation a))(float) {
    switch (a) {
    case ACT_RELU: return act_relu;
    case ACT_SIGMOID: return act_sigmoid;
    case ACT_TANH: return act_tanh;
    case ACT_LINEAR:
    default: return act_identity;
    }
}

static float (*act_grad_fn(Activation a))(float) {
    switch (a) {
    case ACT_RELU: return act_relu_grad;
    case ACT_SIGMOID: return act_sigmoid_grad;
    case ACT_TANH: return act_tanh_grad;
    case ACT_LINEAR:
    default: return act_identity_grad;
    }
}

/* ── Dense layer ────────────────────────────────────────────────────────── */

ErrorCode dense_init(DenseLayer *l, size_t in_dim, size_t out_dim, Activation act, Rng *rng) {
    if (!l || in_dim == 0 || out_dim == 0 || !rng) return ERR_INVALID_ARG;
    memset(l, 0, sizeof(*l));
    l->in_dim = in_dim;
    l->out_dim = out_dim;
    l->act = act;

    ErrorCode err = matx_init(&l->w, in_dim, out_dim);
    if (!err) err = matx_init(&l->b, 1, out_dim);
    if (!err) err = matx_init(&l->dw, in_dim, out_dim);
    if (!err) err = matx_init(&l->db, 1, out_dim);
    if (err) {
        dense_destroy(l);
        return err;
    }

    /* He init for ReLU (std = sqrt(2/in)), Xavier otherwise (std = sqrt(1/in)). */
    float std = act == ACT_RELU ? sqrtf(2.0f / (float)in_dim) : sqrtf(1.0f / (float)in_dim);
    for (size_t i = 0; i < in_dim * out_dim; i++) l->w.data[i] = rng_normal(rng) * std;
    return ERR_OK;
}

void dense_destroy(DenseLayer *l) {
    if (!l) return;
    matx_destroy(&l->w);
    matx_destroy(&l->b);
    matx_destroy(&l->dw);
    matx_destroy(&l->db);
    matx_destroy(&l->x_cache);
    matx_destroy(&l->z_cache);
}

/** (Re)allocate a heap-backed matrix if its shape doesn't match. */
static ErrorCode ensure_shape(MatX *m, size_t rows, size_t cols) {
    if (m->data && m->rows == rows && m->cols == cols) return ERR_OK;
    matx_destroy(m);
    return matx_init(m, rows, cols);
}

ErrorCode dense_forward(DenseLayer *l, const MatX *x, MatX *out) {
    if (!l || !x || !x->data || !out || !out->data) return ERR_INVALID_ARG;
    if (x->cols != l->in_dim) return ERR_INVALID_ARG;
    if (out->rows != x->rows || out->cols != l->out_dim) return ERR_INVALID_ARG;

    size_t    batch = x->rows;
    ErrorCode err = ensure_shape(&l->x_cache, batch, l->in_dim);
    if (!err) err = ensure_shape(&l->z_cache, batch, l->out_dim);
    if (err) return err;

    memcpy(l->x_cache.data, x->data, batch * l->in_dim * sizeof(float));

    /* z = x·W + b (broadcast), a = act(z) */
    err = matx_mul(x, &l->w, &l->z_cache);
    if (!err) err = matx_add_row(&l->z_cache, &l->b, &l->z_cache);
    if (!err) err = matx_map(&l->z_cache, act_fn(l->act), out);
    return err;
}

ErrorCode dense_backward(DenseLayer *l, const MatX *dout, MatX *dx) {
    if (!l || !dout || !dout->data || !l->x_cache.data || !l->z_cache.data) return ERR_INVALID_ARG;
    size_t batch = l->x_cache.rows;
    if (dout->rows != batch || dout->cols != l->out_dim) return ERR_INVALID_ARG;
    if (dx && (dx->rows != batch || dx->cols != l->in_dim)) return ERR_INVALID_ARG;

    MatX      dz = {0}, xt = {0}, wt = {0};
    ErrorCode err = matx_init(&dz, batch, l->out_dim);

    /* dz = dout ⊙ act'(z) */
    if (!err) err = matx_map(&l->z_cache, act_grad_fn(l->act), &dz);
    if (!err) err = matx_hadamard(&dz, dout, &dz);

    /* dW = xᵀ·dz, db = column sums of dz */
    if (!err) err = matx_init(&xt, l->in_dim, batch);
    if (!err) err = matx_transpose(&l->x_cache, &xt);
    if (!err) err = matx_mul(&xt, &dz, &l->dw);
    if (!err) err = matx_col_sums(&dz, &l->db);

    /* dx = dz·Wᵀ (skipped for the first layer) */
    if (!err && dx) {
        err = matx_init(&wt, l->out_dim, l->in_dim);
        if (!err) err = matx_transpose(&l->w, &wt);
        if (!err) err = matx_mul(&dz, &wt, dx);
    }

    matx_destroy(&dz);
    matx_destroy(&xt);
    matx_destroy(&wt);
    return err;
}

ErrorCode dense_sgd_step(DenseLayer *l, float lr) {
    if (!l || !l->w.data || !l->dw.data || lr <= 0.0f) return ERR_INVALID_ARG;
    for (size_t i = 0; i < l->w.rows * l->w.cols; i++) l->w.data[i] -= lr * l->dw.data[i];
    for (size_t i = 0; i < l->b.cols; i++) l->b.data[i] -= lr * l->db.data[i];
    return ERR_OK;
}

/* ── Adam ───────────────────────────────────────────────────────────────── */

ErrorCode adam_init(AdamState *s, const DenseLayer *l) {
    if (!s || !l || !l->w.data) return ERR_INVALID_ARG;
    memset(s, 0, sizeof(*s));

    ErrorCode err = matx_init(&s->mw, l->w.rows, l->w.cols);
    if (!err) err = matx_init(&s->vw, l->w.rows, l->w.cols);
    if (!err) err = matx_init(&s->mb, 1, l->b.cols);
    if (!err) err = matx_init(&s->vb, 1, l->b.cols);
    if (err) adam_destroy(s);
    return err;
}

void adam_destroy(AdamState *s) {
    if (!s) return;
    matx_destroy(&s->mw);
    matx_destroy(&s->vw);
    matx_destroy(&s->mb);
    matx_destroy(&s->vb);
}

/** One Adam update over a flat parameter array. */
static void adam_update(float *param, const float *grad, float *m, float *v, size_t n, float lr,
                        int t) {
    float bc1 = 1.0f - powf(ADAM_BETA1, (float)t);
    float bc2 = 1.0f - powf(ADAM_BETA2, (float)t);
    for (size_t i = 0; i < n; i++) {
        m[i] = ADAM_BETA1 * m[i] + (1.0f - ADAM_BETA1) * grad[i];
        v[i] = ADAM_BETA2 * v[i] + (1.0f - ADAM_BETA2) * grad[i] * grad[i];
        float mhat = m[i] / bc1;
        float vhat = v[i] / bc2;
        param[i] -= lr * mhat / (sqrtf(vhat) + ADAM_EPS);
    }
}

ErrorCode adam_step(AdamState *s, DenseLayer *l, float lr) {
    if (!s || !s->mw.data || !l || !l->w.data || lr <= 0.0f) return ERR_INVALID_ARG;
    if (s->mw.rows != l->w.rows || s->mw.cols != l->w.cols) return ERR_INVALID_ARG;

    s->t++;
    adam_update(l->w.data, l->dw.data, s->mw.data, s->vw.data, l->w.rows * l->w.cols, lr, s->t);
    adam_update(l->b.data, l->db.data, s->mb.data, s->vb.data, l->b.cols, lr, s->t);
    return ERR_OK;
}

/* ── Losses ─────────────────────────────────────────────────────────────── */

ErrorCode loss_mse(const MatX *pred, const MatX *target, float *out_loss, MatX *dpred) {
    if (!pred || !target || !pred->data || !target->data) return ERR_INVALID_ARG;
    if (pred->rows != target->rows || pred->cols != target->cols) return ERR_INVALID_ARG;
    if (dpred && (dpred->rows != pred->rows || dpred->cols != pred->cols)) return ERR_INVALID_ARG;

    size_t n = pred->rows * pred->cols;
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        float diff = pred->data[i] - target->data[i];
        sum += (double)(diff * diff);
        if (dpred) dpred->data[i] = 2.0f * diff / (float)n;
    }
    if (out_loss) *out_loss = (float)(sum / (double)n);
    return ERR_OK;
}

ErrorCode loss_softmax_xent(const MatX *logits, const MatX *onehot, float *out_loss,
                            MatX *dlogits) {
    if (!logits || !onehot || !logits->data || !onehot->data) return ERR_INVALID_ARG;
    if (logits->rows != onehot->rows || logits->cols != onehot->cols) return ERR_INVALID_ARG;
    if (dlogits && (dlogits->rows != logits->rows || dlogits->cols != logits->cols))
        return ERR_INVALID_ARG;

    size_t batch = logits->rows, classes = logits->cols;
    double total = 0.0;

    for (size_t r = 0; r < batch; r++) {
        const float *row = logits->data + r * classes;

        /* Stable softmax: subtract the row max before exponentiating. */
        float max = row[0];
        for (size_t c = 1; c < classes; c++)
            if (row[c] > max) max = row[c];
        float sum = 0.0f;
        for (size_t c = 0; c < classes; c++) sum += expf(row[c] - max);
        float log_sum = logf(sum) + max;

        for (size_t c = 0; c < classes; c++) {
            float p = expf(row[c] - log_sum);
            float y = onehot->data[r * classes + c];
            if (y > 0.0f) total += (double)(y * (log_sum - row[c]));
            if (dlogits) dlogits->data[r * classes + c] = (p - y) / (float)batch;
        }
    }
    if (out_loss) *out_loss = (float)(total / (double)batch);
    return ERR_OK;
}
