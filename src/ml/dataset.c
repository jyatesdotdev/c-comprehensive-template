/**
 * @file dataset.c
 * @brief Dataset utilities implementation.
 */
#include "ml/dataset.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CSV_LINE_MAX 4096

/** Count comma-separated fields in a line. */
static size_t count_fields(const char *line) {
    size_t n = 1;
    for (const char *p = line; *p; p++)
        if (*p == ',') n++;
    return n;
}

ErrorCode dataset_load_csv(const char *path, int has_header, MatX *out) {
    if (!path || !out) return ERR_INVALID_ARG;

    FILE *f = fopen(path, "r");
    if (!f) return ERR_NOT_FOUND;

    char      line[CSV_LINE_MAX];
    float    *vals = NULL;
    size_t    count = 0, cap = 0, cols = 0, rows = 0;
    ErrorCode err = ERR_OK;
    int       skipped_header = !has_header;

    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        if (len + 1 >= sizeof(line)) {
            err = ERR_UNSUPPORTED; /* line longer than CSV_LINE_MAX */
            break;
        }
        if (!skipped_header) {
            skipped_header = 1;
            continue;
        }
        /* Skip blank lines (e.g. trailing newline at EOF). */
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') continue;

        size_t fields = count_fields(line);
        if (cols == 0) cols = fields;
        if (fields != cols) {
            err = ERR_IO; /* inconsistent column count */
            break;
        }

        if (count + cols > cap) {
            size_t new_cap = cap == 0 ? 256 : cap * 2;
            while (new_cap < count + cols) new_cap *= 2;
            float *tmp = realloc(vals, new_cap * sizeof(float));
            if (!tmp) {
                err = ERR_NOMEM;
                break;
            }
            vals = tmp;
            cap = new_cap;
        }

        char *p = line;
        for (size_t c = 0; c < cols; c++) {
            char *end = NULL;
            vals[count++] = strtof(p, &end);
            if (end == p) {
                err = ERR_IO; /* not a number */
                break;
            }
            p = end;
            while (*p == ',' || *p == ' ') p++;
        }
        if (err) break;
        rows++;
    }
    (void)fclose(f);

    if (!err && rows == 0) err = ERR_IO; /* empty file */
    if (!err) err = matx_init(out, rows, cols);
    if (!err) memcpy(out->data, vals, rows * cols * sizeof(float));
    free(vals);
    return err;
}

ErrorCode dataset_shuffle(MatX *x, MatX *y, Rng *rng) {
    if (!x || !x->data || !rng) return ERR_INVALID_ARG;
    if (y && (!y->data || y->rows != x->rows)) return ERR_INVALID_ARG;

    size_t max_cols = x->cols;
    if (y && y->cols > max_cols) max_cols = y->cols;
    float *tmp = malloc(max_cols * sizeof(float));
    if (!tmp) return ERR_NOMEM;

    for (size_t i = x->rows - 1; i > 0; i--) {
        size_t j = rng_range_u32(rng, (uint32_t)(i + 1));
        if (j == i) continue;

        size_t xb = x->cols * sizeof(float);
        memcpy(tmp, x->data + i * x->cols, xb);
        memcpy(x->data + i * x->cols, x->data + j * x->cols, xb);
        memcpy(x->data + j * x->cols, tmp, xb);

        if (y) {
            size_t yb = y->cols * sizeof(float);
            memcpy(tmp, y->data + i * y->cols, yb);
            memcpy(y->data + i * y->cols, y->data + j * y->cols, yb);
            memcpy(y->data + j * y->cols, tmp, yb);
        }
    }
    free(tmp);
    return ERR_OK;
}

ErrorCode dataset_batch(const MatX *src, size_t start, size_t count, MatX *dst) {
    if (!src || !src->data || !dst || !dst->data || count == 0) return ERR_INVALID_ARG;
    if (start + count > src->rows) return ERR_INVALID_ARG;
    if (dst->rows != count || dst->cols != src->cols) return ERR_INVALID_ARG;

    memcpy(dst->data, src->data + start * src->cols, count * src->cols * sizeof(float));
    return ERR_OK;
}

ErrorCode dataset_normalize_zscore(MatX *x) {
    if (!x || !x->data || x->rows < 2) return ERR_INVALID_ARG;

    for (size_t c = 0; c < x->cols; c++) {
        /* Welford over the column. */
        double mean = 0.0, m2 = 0.0;
        for (size_t r = 0; r < x->rows; r++) {
            double v = (double)matx_get(x, r, c);
            double delta = v - mean;
            mean += delta / (double)(r + 1);
            m2 += delta * (v - mean);
        }
        double var = m2 / (double)(x->rows - 1);
        float  std = (float)sqrt(var);
        float  fmean = (float)mean;

        for (size_t r = 0; r < x->rows; r++) {
            float centered = matx_get(x, r, c) - fmean;
            matx_set(x, r, c, std > 1e-12f ? centered / std : centered);
        }
    }
    return ERR_OK;
}

ErrorCode dataset_one_hot(const MatX *labels, size_t classes, MatX *out) {
    if (!labels || !labels->data || labels->cols != 1 || classes == 0) return ERR_INVALID_ARG;
    if (!out || !out->data || out->rows != labels->rows || out->cols != classes)
        return ERR_INVALID_ARG;

    memset(out->data, 0, out->rows * out->cols * sizeof(float));
    for (size_t r = 0; r < labels->rows; r++) {
        float v = matx_get(labels, r, 0);
        if (v < 0.0f || v >= (float)classes) return ERR_INVALID_ARG;
        matx_set(out, r, (size_t)v, 1.0f);
    }
    return ERR_OK;
}

ErrorCode dataset_accuracy(const MatX *pred, const MatX *target, float *out_acc) {
    if (!pred || !target || !pred->data || !target->data || !out_acc) return ERR_INVALID_ARG;
    if (pred->rows != target->rows || pred->cols != target->cols) return ERR_INVALID_ARG;
    if (pred->rows == 0) return ERR_INVALID_ARG;

    size_t correct = 0;
    for (size_t r = 0; r < pred->rows; r++) {
        size_t    p = 0, t = 0;
        ErrorCode err = matx_row_argmax(pred, r, &p);
        if (!err) err = matx_row_argmax(target, r, &t);
        if (err) return err;
        if (p == t) correct++;
    }
    *out_acc = (float)correct / (float)pred->rows;
    return ERR_OK;
}
