/**
 * @file ml_demo.c
 * @brief End-to-end ML pipeline: generate a 3-class spiral dataset, train a
 *        two-layer MLP with Adam, and report loss + accuracy.
 *
 * Composes the template's pieces: math/matx (linear algebra), math/rng
 * (reproducible init + shuffling), ml/nn (layers, loss, optimizer), and
 * ml/dataset (shuffle, batch, one-hot, accuracy).
 */
#include "ml/dataset.h"
#include "ml/nn.h"
#include "math/scalar.h"

#include <math.h>
#include <stdio.h>

enum {
    CLASSES = 3,
    PER_CLASS = 100,
    SAMPLES = CLASSES * PER_CLASS,
    HIDDEN = 32,
    EPOCHS = 300,
    BATCH = 50,
};

/** Classic interleaved-spirals dataset: hard for linear models, easy visually. */
static void make_spiral(MatX *x, MatX *labels, Rng *rng) {
    for (int c = 0; c < CLASSES; c++) {
        for (int i = 0; i < PER_CLASS; i++) {
            size_t row = (size_t)c * PER_CLASS + (size_t)i;
            float  t = (float)i / (float)PER_CLASS;
            float  radius = t;
            float  angle = (float)c * 4.0f + t * 4.0f + rng_normal(rng) * 0.2f;
            matx_set(x, row, 0, radius * sinf(angle));
            matx_set(x, row, 1, radius * cosf(angle));
            matx_set(labels, row, 0, (float)c);
        }
    }
}

int main(void) {
    Rng rng;
    rng_seed(&rng, 42, 0);

    /* ── Data ──────────────────────────────────────────────────────────── */
    MatX x, labels, y;
    if (matx_init(&x, SAMPLES, 2) || matx_init(&labels, SAMPLES, 1) ||
        matx_init(&y, SAMPLES, CLASSES)) {
        fprintf(stderr, "allocation failed\n");
        return 1;
    }
    make_spiral(&x, &labels, &rng);
    dataset_one_hot(&labels, CLASSES, &y);
    dataset_shuffle(&x, &y, &rng);

    /* ── Model: 2 → HIDDEN (tanh) → CLASSES (linear + softmax-xent) ────── */
    DenseLayer l1, l2;
    AdamState  a1, a2;
    if (dense_init(&l1, 2, HIDDEN, ACT_TANH, &rng) ||
        dense_init(&l2, HIDDEN, CLASSES, ACT_LINEAR, &rng) || adam_init(&a1, &l1) ||
        adam_init(&a2, &l2)) {
        fprintf(stderr, "model init failed\n");
        return 1;
    }

    MatX bx, by, h, logits, dlogits, dh;
    if (matx_init(&bx, BATCH, 2) || matx_init(&by, BATCH, CLASSES) ||
        matx_init(&h, BATCH, HIDDEN) || matx_init(&logits, BATCH, CLASSES) ||
        matx_init(&dlogits, BATCH, CLASSES) || matx_init(&dh, BATCH, HIDDEN)) {
        fprintf(stderr, "batch allocation failed\n");
        return 1;
    }

    /* ── Training loop ─────────────────────────────────────────────────── */
    printf("Training 2-%d-%d MLP on %d-sample spiral (Adam, batch %d)\n", HIDDEN, CLASSES, SAMPLES,
           BATCH);
    for (int epoch = 1; epoch <= EPOCHS; epoch++) {
        float epoch_loss = 0.0f;
        int   batches = 0;

        for (size_t start = 0; start + BATCH <= SAMPLES; start += BATCH) {
            dataset_batch(&x, start, BATCH, &bx);
            dataset_batch(&y, start, BATCH, &by);

            float loss = 0.0f;
            dense_forward(&l1, &bx, &h);
            dense_forward(&l2, &h, &logits);
            loss_softmax_xent(&logits, &by, &loss, &dlogits);
            dense_backward(&l2, &dlogits, &dh);
            dense_backward(&l1, &dh, NULL);
            adam_step(&a2, &l2, 0.01f);
            adam_step(&a1, &l1, 0.01f);

            epoch_loss += loss;
            batches++;
        }
        if (epoch == 1 || epoch % 50 == 0)
            printf("  epoch %3d  loss %.4f\n", epoch, (double)(epoch_loss / (float)batches));
    }

    /* ── Evaluation on the full dataset ────────────────────────────────── */
    MatX full_h, full_logits;
    if (matx_init(&full_h, SAMPLES, HIDDEN) || matx_init(&full_logits, SAMPLES, CLASSES)) {
        fprintf(stderr, "eval allocation failed\n");
        return 1;
    }
    dense_forward(&l1, &x, &full_h);
    dense_forward(&l2, &full_h, &full_logits);
    float acc = 0.0f;
    dataset_accuracy(&full_logits, &y, &acc);
    printf("Final accuracy: %.1f%% (%d samples)\n", (double)(acc * 100.0f), SAMPLES);

    matx_destroy(&full_h);
    matx_destroy(&full_logits);
    matx_destroy(&bx);
    matx_destroy(&by);
    matx_destroy(&h);
    matx_destroy(&logits);
    matx_destroy(&dlogits);
    matx_destroy(&dh);
    adam_destroy(&a1);
    adam_destroy(&a2);
    dense_destroy(&l1);
    dense_destroy(&l2);
    matx_destroy(&x);
    matx_destroy(&labels);
    matx_destroy(&y);
    return acc > 0.9f ? 0 : 1;
}
