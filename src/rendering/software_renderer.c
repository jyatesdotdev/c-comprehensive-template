/**
 * @file software_renderer.c
 * @brief Software framebuffer renderer with basic 2D drawing primitives.
 */
#include "rendering/software_renderer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ── Lifecycle ─────────────────────────────────────────────────────────── */

ErrorCode fb_create(Framebuffer *fb, int width, int height) {
    if (!fb || width <= 0 || height <= 0) return ERR_INVALID_ARG;
    fb->pixels = calloc((size_t)width * (size_t)height, sizeof(uint32_t));
    if (!fb->pixels) return ERR_NOMEM;
    fb->width = width;
    fb->height = height;
    return ERR_OK;
}

void fb_destroy(Framebuffer *fb) {
    if (fb) {
        free(fb->pixels);
        fb->pixels = NULL;
    }
}

/* ── Drawing ───────────────────────────────────────────────────────────── */

void fb_clear(Framebuffer *fb, uint32_t color) {
    if (!fb || !fb->pixels) return;
    size_t n = (size_t)fb->width * (size_t)fb->height;
    for (size_t i = 0; i < n; i++) fb->pixels[i] = color;
}

void fb_set_pixel(Framebuffer *fb, int x, int y, uint32_t color) {
    if (!fb || !fb->pixels) return;
    if (x < 0 || x >= fb->width || y < 0 || y >= fb->height) return;
    fb->pixels[y * fb->width + x] = color;
}

/* Bresenham's line algorithm */
void fb_line(Framebuffer *fb, int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        fb_set_pixel(fb, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void fb_fill_rect(Framebuffer *fb, int x, int y, int w, int h, uint32_t color) {
    for (int row = y; row < y + h; row++)
        for (int col = x; col < x + w; col++) fb_set_pixel(fb, col, row, color);
}

/* Midpoint circle algorithm */
void fb_circle(Framebuffer *fb, int cx, int cy, int r, uint32_t color) {
    int x = r, y = 0, d = 1 - r;
    while (x >= y) {
        fb_set_pixel(fb, cx + x, cy + y, color);
        fb_set_pixel(fb, cx - x, cy + y, color);
        fb_set_pixel(fb, cx + x, cy - y, color);
        fb_set_pixel(fb, cx - x, cy - y, color);
        fb_set_pixel(fb, cx + y, cy + x, color);
        fb_set_pixel(fb, cx - y, cy + x, color);
        fb_set_pixel(fb, cx + y, cy - x, color);
        fb_set_pixel(fb, cx - y, cy - x, color);
        y++;
        if (d <= 0) {
            d += 2 * y + 1;
        } else {
            x--;
            d += 2 * (y - x) + 1;
        }
    }
}

/* ── Output ────────────────────────────────────────────────────────────── */

ErrorCode fb_write_ppm(const Framebuffer *fb, const char *path) {
    if (!fb || !fb->pixels || !path) return ERR_INVALID_ARG;
    FILE *f = fopen(path, "wb");
    if (!f) return ERR_IO;
    fprintf(f, "P6\n%d %d\n255\n", fb->width, fb->height);
    size_t n = (size_t)fb->width * (size_t)fb->height;
    for (size_t i = 0; i < n; i++) {
        uint32_t p = fb->pixels[i];
        uint8_t  rgb[3] = {(uint8_t)(p >> 16), (uint8_t)(p >> 8), (uint8_t)p};
        if (fwrite(rgb, 1, 3, f) != 3) {
            (void)fclose(f); /* already returning an error */
            return ERR_IO;
        }
    }
    /* fclose flushes buffered data — a failure here means the write failed. */
    if (fclose(f) != 0) return ERR_IO;
    return ERR_OK;
}
