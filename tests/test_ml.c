/**
 * @file test_ml.c
 * @brief Tests for the ml module: layers, losses, optimizers, dataset utils.
 *
 * The dense-layer backward pass is verified against numerical gradients
 * (finite differences) — the gold-standard check for backprop code.
 */
#include "ml/dataset.h"
#include "ml/nn.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

/* ── Losses ─────────────────────────────────────────────────────────────── */

static void test_losses(void) {
    MatX pred, target, grad;
    CHECK(matx_init(&pred, 2, 2) == ERR_OK);
    CHECK(matx_init(&target, 2, 2) == ERR_OK);
    CHECK(matx_init(&grad, 2, 2) == ERR_OK);

    /* MSE of all-ones vs all-zeros is 1; gradient is 2*diff/n = 0.5 */
    CHECK(matx_fill(&pred, 1.0f) == ERR_OK);
    float loss = 0.0f;
    CHECK(loss_mse(&pred, &target, &loss, &grad) == ERR_OK);
    CHECK(fabsf(loss - 1.0f) < 1e-6f);
    CHECK(fabsf(grad.data[0] - 0.5f) < 1e-6f);

    /* Perfect predictions → zero loss */
    CHECK(loss_mse(&target, &target, &loss, NULL) == ERR_OK);
    CHECK(loss < 1e-9f);

    /* Softmax-xent: uniform logits over 2 classes → loss = ln(2) */
    CHECK(matx_fill(&pred, 3.0f) == ERR_OK);
    matx_set(&target, 0, 0, 1.0f);
    matx_set(&target, 1, 1, 1.0f);
    CHECK(loss_softmax_xent(&pred, &target, &loss, &grad) == ERR_OK);
    CHECK(fabsf(loss - logf(2.0f)) < 1e-5f);
    /* gradient rows sum to ~0: (p - y) with p summing to 1, y summing to 1 */
    CHECK(fabsf(grad.data[0] + grad.data[1]) < 1e-6f);

    /* Confident correct logits → small loss */
    matx_set(&pred, 0, 0, 10.0f);
    matx_set(&pred, 0, 1, -10.0f);
    matx_set(&pred, 1, 0, -10.0f);
    matx_set(&pred, 1, 1, 10.0f);
    CHECK(loss_softmax_xent(&pred, &target, &loss, NULL) == ERR_OK);
    CHECK(loss < 1e-3f);

    /* Dimension mismatch rejected */
    MatX bad;
    CHECK(matx_init(&bad, 1, 2) == ERR_OK);
    CHECK(loss_mse(&pred, &bad, &loss, NULL) == ERR_INVALID_ARG);
    matx_destroy(&bad);

    matx_destroy(&pred);
    matx_destroy(&target);
    matx_destroy(&grad);
}

/* ── Gradient check: analytic backward vs finite differences ────────────── */

/** Forward through one layer and return MSE loss against target. */
static float layer_loss(DenseLayer *l, const MatX *x, const MatX *target, MatX *out) {
    float loss = 0.0f;
    CHECK(dense_forward(l, x, out) == ERR_OK);
    CHECK(loss_mse(out, target, &loss, NULL) == ERR_OK);
    return loss;
}

static void gradient_check_activation(Activation act) {
    enum { BATCH = 4, IN = 3, OUT = 2 };
    Rng rng;
    rng_seed(&rng, 7, (uint64_t)act);

    DenseLayer l;
    CHECK(dense_init(&l, IN, OUT, act, &rng) == ERR_OK);

    MatX x, target, out, dout;
    CHECK(matx_init(&x, BATCH, IN) == ERR_OK);
    CHECK(matx_init(&target, BATCH, OUT) == ERR_OK);
    CHECK(matx_init(&out, BATCH, OUT) == ERR_OK);
    CHECK(matx_init(&dout, BATCH, OUT) == ERR_OK);
    CHECK(matx_random_uniform(&x, &rng, -1.0f, 1.0f) == ERR_OK);
    CHECK(matx_random_uniform(&target, &rng, -1.0f, 1.0f) == ERR_OK);

    /* Analytic gradients */
    CHECK(dense_forward(&l, &x, &out) == ERR_OK);
    CHECK(loss_mse(&out, &target, NULL, &dout) == ERR_OK);
    CHECK(dense_backward(&l, &dout, NULL) == ERR_OK);

    /* Numerical gradients via central differences on every weight + bias */
    const float h = 1e-3f;
    for (size_t i = 0; i < l.w.rows * l.w.cols; i++) {
        float saved = l.w.data[i];
        l.w.data[i] = saved + h;
        float lp = layer_loss(&l, &x, &target, &out);
        l.w.data[i] = saved - h;
        float lm = layer_loss(&l, &x, &target, &out);
        l.w.data[i] = saved;

        float numeric = (lp - lm) / (2.0f * h);
        float analytic = l.dw.data[i];
        CHECK(fabsf(numeric - analytic) < 1e-2f * fmaxf(1.0f, fabsf(numeric)));
    }
    for (size_t i = 0; i < l.b.cols; i++) {
        float saved = l.b.data[i];
        l.b.data[i] = saved + h;
        float lp = layer_loss(&l, &x, &target, &out);
        l.b.data[i] = saved - h;
        float lm = layer_loss(&l, &x, &target, &out);
        l.b.data[i] = saved;

        float numeric = (lp - lm) / (2.0f * h);
        CHECK(fabsf(numeric - l.db.data[i]) < 1e-2f * fmaxf(1.0f, fabsf(numeric)));
    }

    matx_destroy(&x);
    matx_destroy(&target);
    matx_destroy(&out);
    matx_destroy(&dout);
    dense_destroy(&l);
}

static void test_gradient_check(void) {
    gradient_check_activation(ACT_LINEAR);
    gradient_check_activation(ACT_SIGMOID);
    gradient_check_activation(ACT_TANH);
    /* ReLU is excluded: its kink makes finite differences unreliable near 0. */
}

/* ── Training: 2-layer MLP learns XOR ───────────────────────────────────── */

static void test_xor_convergence(void) {
    /* clang-format off */
    float xor_x[] = {0, 0,  0, 1,  1, 0,  1, 1};
    float xor_y[] = {0, 1, 1, 0};
    /* clang-format on */

    MatX x, y, h, out, dh, dout;
    CHECK(matx_init(&x, 4, 2) == ERR_OK);
    CHECK(matx_init(&y, 4, 1) == ERR_OK);
    CHECK(matx_init(&h, 4, 8) == ERR_OK);
    CHECK(matx_init(&out, 4, 1) == ERR_OK);
    CHECK(matx_init(&dh, 4, 8) == ERR_OK);
    CHECK(matx_init(&dout, 4, 1) == ERR_OK);
    memcpy(x.data, xor_x, sizeof(xor_x));
    memcpy(y.data, xor_y, sizeof(xor_y));

    Rng rng;
    rng_seed(&rng, 1234, 0);
    DenseLayer l1, l2;
    CHECK(dense_init(&l1, 2, 8, ACT_TANH, &rng) == ERR_OK);
    CHECK(dense_init(&l2, 8, 1, ACT_SIGMOID, &rng) == ERR_OK);

    AdamState a1, a2;
    CHECK(adam_init(&a1, &l1) == ERR_OK);
    CHECK(adam_init(&a2, &l2) == ERR_OK);

    float loss = 1e9f;
    for (int epoch = 0; epoch < 500; epoch++) {
        CHECK(dense_forward(&l1, &x, &h) == ERR_OK);
        CHECK(dense_forward(&l2, &h, &out) == ERR_OK);
        CHECK(loss_mse(&out, &y, &loss, &dout) == ERR_OK);
        CHECK(dense_backward(&l2, &dout, &dh) == ERR_OK);
        CHECK(dense_backward(&l1, &dh, NULL) == ERR_OK);
        CHECK(adam_step(&a2, &l2, 0.01f) == ERR_OK);
        CHECK(adam_step(&a1, &l1, 0.01f) == ERR_OK);
    }
    CHECK(loss < 0.01f); /* XOR is learned */

    /* Every sample classified correctly at threshold 0.5 */
    CHECK(dense_forward(&l1, &x, &h) == ERR_OK);
    CHECK(dense_forward(&l2, &h, &out) == ERR_OK);
    for (int i = 0; i < 4; i++) {
        float p = out.data[i] > 0.5f ? 1.0f : 0.0f;
        CHECK(p == xor_y[i]);
    }

    /* SGD also reduces loss (smoke test for the plain optimizer) */
    DenseLayer l3;
    CHECK(dense_init(&l3, 2, 1, ACT_SIGMOID, &rng) == ERR_OK);
    MatX out3, dout3;
    CHECK(matx_init(&out3, 4, 1) == ERR_OK);
    CHECK(matx_init(&dout3, 4, 1) == ERR_OK);
    float first = 0.0f, last = 0.0f;
    for (int i = 0; i < 50; i++) {
        CHECK(dense_forward(&l3, &x, &out3) == ERR_OK);
        CHECK(loss_mse(&out3, &y, &last, &dout3) == ERR_OK);
        if (i == 0) first = last;
        CHECK(dense_backward(&l3, &dout3, NULL) == ERR_OK);
        CHECK(dense_sgd_step(&l3, 0.5f) == ERR_OK);
    }
    CHECK(last < first);

    adam_destroy(&a1);
    adam_destroy(&a2);
    dense_destroy(&l1);
    dense_destroy(&l2);
    dense_destroy(&l3);
    matx_destroy(&x);
    matx_destroy(&y);
    matx_destroy(&h);
    matx_destroy(&out);
    matx_destroy(&dh);
    matx_destroy(&dout);
    matx_destroy(&out3);
    matx_destroy(&dout3);
}

/* ── Dataset utilities ──────────────────────────────────────────────────── */

static void test_dataset_csv(void) {
    const char *path = "test_ml_data.csv";
    FILE       *f = fopen(path, "w");
    CHECK(f != NULL);
    fprintf(f, "a,b,label\n1.0,2.0,0\n3.0,4.0,1\n5.5,-1.5,2\n");
    CHECK(fclose(f) == 0);

    MatX m;
    CHECK(dataset_load_csv(path, 1, &m) == ERR_OK);
    CHECK(m.rows == 3 && m.cols == 3);
    CHECK(fabsf(matx_get(&m, 0, 0) - 1.0f) < 1e-6f);
    CHECK(fabsf(matx_get(&m, 2, 1) + 1.5f) < 1e-6f);
    CHECK(fabsf(matx_get(&m, 2, 2) - 2.0f) < 1e-6f);
    matx_destroy(&m);

    /* Header not skipped → text cells fail to parse */
    CHECK(dataset_load_csv(path, 0, &m) == ERR_IO);
    CHECK(remove(path) == 0);
    CHECK(dataset_load_csv("no_such_file.csv", 0, &m) == ERR_NOT_FOUND);
}

static void test_dataset_utils(void) {
    Rng rng;
    rng_seed(&rng, 5, 5);

    /* Shuffle keeps x/y rows paired: y holds each row's original index */
    MatX x, y;
    CHECK(matx_init(&x, 16, 2) == ERR_OK);
    CHECK(matx_init(&y, 16, 1) == ERR_OK);
    for (size_t r = 0; r < 16; r++) {
        matx_set(&x, r, 0, (float)r);
        matx_set(&x, r, 1, (float)r * 10.0f);
        matx_set(&y, r, 0, (float)r);
    }
    CHECK(dataset_shuffle(&x, &y, &rng) == ERR_OK);
    int moved = 0;
    for (size_t r = 0; r < 16; r++) {
        float orig = matx_get(&y, r, 0);
        CHECK(matx_get(&x, r, 0) == orig);
        CHECK(matx_get(&x, r, 1) == orig * 10.0f);
        if ((size_t)orig != r) moved = 1;
    }
    CHECK(moved); /* the permutation actually permuted something */

    /* Batching copies the right rows */
    MatX batch;
    CHECK(matx_init(&batch, 4, 2) == ERR_OK);
    CHECK(dataset_batch(&x, 4, 4, &batch) == ERR_OK);
    CHECK(matx_get(&batch, 0, 0) == matx_get(&x, 4, 0));
    CHECK(dataset_batch(&x, 14, 4, &batch) == ERR_INVALID_ARG); /* out of range */
    matx_destroy(&batch);

    /* Z-score: each column ends up mean ~0, sample std ~1 */
    CHECK(dataset_normalize_zscore(&x) == ERR_OK);
    for (size_t c = 0; c < 2; c++) {
        float sum = 0.0f, sq = 0.0f;
        for (size_t r = 0; r < 16; r++) {
            float v = matx_get(&x, r, c);
            sum += v;
            sq += v * v;
        }
        CHECK(fabsf(sum / 16.0f) < 1e-5f);
        CHECK(fabsf(sq / 15.0f - 1.0f) < 1e-4f);
    }

    /* One-hot + accuracy */
    MatX labels, onehot;
    CHECK(matx_init(&labels, 3, 1) == ERR_OK);
    CHECK(matx_init(&onehot, 3, 4) == ERR_OK);
    matx_set(&labels, 0, 0, 0.0f);
    matx_set(&labels, 1, 0, 3.0f);
    matx_set(&labels, 2, 0, 1.0f);
    CHECK(dataset_one_hot(&labels, 4, &onehot) == ERR_OK);
    CHECK(matx_get(&onehot, 1, 3) == 1.0f);
    CHECK(matx_get(&onehot, 1, 0) == 0.0f);
    matx_set(&labels, 2, 0, 9.0f); /* out of range */
    CHECK(dataset_one_hot(&labels, 4, &onehot) == ERR_INVALID_ARG);

    MatX pred;
    CHECK(matx_init(&pred, 3, 4) == ERR_OK);
    matx_set(&pred, 0, 0, 5.0f); /* correct */
    matx_set(&pred, 1, 3, 5.0f); /* correct */
    matx_set(&pred, 2, 2, 5.0f); /* wrong (target is class 1) */
    matx_set(&labels, 2, 0, 1.0f);
    CHECK(dataset_one_hot(&labels, 4, &onehot) == ERR_OK);
    float acc = 0.0f;
    CHECK(dataset_accuracy(&pred, &onehot, &acc) == ERR_OK);
    CHECK(fabsf(acc - 2.0f / 3.0f) < 1e-6f);

    matx_destroy(&labels);
    matx_destroy(&onehot);
    matx_destroy(&pred);
    matx_destroy(&x);
    matx_destroy(&y);
}

/* ── Invalid arguments ──────────────────────────────────────────────────── */

static void test_invalid_args(void) {
    Rng rng;
    rng_seed(&rng, 1, 1);
    DenseLayer l;
    CHECK(dense_init(NULL, 2, 2, ACT_RELU, &rng) == ERR_INVALID_ARG);
    CHECK(dense_init(&l, 0, 2, ACT_RELU, &rng) == ERR_INVALID_ARG);
    CHECK(dense_init(&l, 2, 2, ACT_RELU, NULL) == ERR_INVALID_ARG);
    CHECK(dense_init(&l, 2, 2, ACT_RELU, &rng) == ERR_OK);

    MatX wrong;
    CHECK(matx_init(&wrong, 1, 3) == ERR_OK); /* 3 != in_dim 2 */
    MatX out;
    CHECK(matx_init(&out, 1, 2) == ERR_OK);
    CHECK(dense_forward(&l, &wrong, &out) == ERR_INVALID_ARG);
    CHECK(dense_backward(&l, &out, NULL) == ERR_INVALID_ARG); /* no forward ran */
    CHECK(dense_sgd_step(&l, 0.0f) == ERR_INVALID_ARG);

    AdamState s;
    CHECK(adam_init(NULL, &l) == ERR_INVALID_ARG);
    CHECK(adam_init(&s, &l) == ERR_OK);
    CHECK(adam_step(&s, &l, -1.0f) == ERR_INVALID_ARG);
    adam_destroy(&s);
    adam_destroy(NULL);

    dense_destroy(&l);
    dense_destroy(NULL);
    matx_destroy(&wrong);
    matx_destroy(&out);
}

int main(void) {
    test_losses();
    test_gradient_check();
    test_xor_convergence();
    test_dataset_csv();
    test_dataset_utils();
    test_invalid_args();
    printf("All ml tests passed.\n");
    return 0;
}
