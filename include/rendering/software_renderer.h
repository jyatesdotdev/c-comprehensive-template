/**
 * @file software_renderer.h
 * @brief Software framebuffer renderer with drawing primitives.
 *
 * No external dependencies — works everywhere. Supports pixel ops,
 * line drawing (Bresenham), filled rectangles, and PPM image output.
 */
#ifndef RENDERING_SOFTWARE_RENDERER_H
#define RENDERING_SOFTWARE_RENDERER_H

#include "core/error.h"
#include <stddef.h>
#include <stdint.h>

/** @brief RGBA framebuffer (pixels stored as 0xAARRGGBB). */
typedef struct {
    uint32_t *pixels;
    int       width;
    int       height;
} Framebuffer;

/* ── Lifecycle ─────────────────────────────────────────────────────────── */

/**
 * @brief Create a framebuffer with the given dimensions.
 * @param fb     Framebuffer to initialize.
 * @param width  Width in pixels.
 * @param height Height in pixels.
 * @return ERR_OK on success, ERR_NOMEM on allocation failure.
 */
ErrorCode fb_create(Framebuffer *fb, int width, int height);

/**
 * @brief Destroy a framebuffer and free its pixel buffer.
 * @param fb Framebuffer to destroy.
 */
void      fb_destroy(Framebuffer *fb);

/* ── Drawing ───────────────────────────────────────────────────────────── */

/**
 * @brief Clear the entire framebuffer to a solid color.
 * @param fb    Framebuffer to clear.
 * @param color Fill color (0xAARRGGBB).
 */
void fb_clear(Framebuffer *fb, uint32_t color);

/**
 * @brief Set a single pixel. Out-of-bounds coordinates are ignored.
 * @param fb    Framebuffer.
 * @param x     X coordinate.
 * @param y     Y coordinate.
 * @param color Pixel color (0xAARRGGBB).
 */
void fb_set_pixel(Framebuffer *fb, int x, int y, uint32_t color);

/**
 * @brief Draw a line using Bresenham's algorithm.
 * @param fb    Framebuffer.
 * @param x0    Start X.
 * @param y0    Start Y.
 * @param x1    End X.
 * @param y1    End Y.
 * @param color Line color (0xAARRGGBB).
 */
void fb_line(Framebuffer *fb, int x0, int y0, int x1, int y1, uint32_t color);

/**
 * @brief Draw a filled rectangle.
 * @param fb    Framebuffer.
 * @param x     Top-left X.
 * @param y     Top-left Y.
 * @param w     Width in pixels.
 * @param h     Height in pixels.
 * @param color Fill color (0xAARRGGBB).
 */
void fb_fill_rect(Framebuffer *fb, int x, int y, int w, int h, uint32_t color);

/**
 * @brief Draw a circle outline using the midpoint algorithm.
 * @param fb    Framebuffer.
 * @param cx    Center X.
 * @param cy    Center Y.
 * @param r     Radius in pixels.
 * @param color Circle color (0xAARRGGBB).
 */
void fb_circle(Framebuffer *fb, int cx, int cy, int r, uint32_t color);

/* ── Output ────────────────────────────────────────────────────────────── */

/**
 * @brief Write framebuffer to a PPM (P6) file.
 * @param fb   Framebuffer to write.
 * @param path Output file path.
 * @return ERR_OK on success, ERR_IO on file error.
 */
ErrorCode fb_write_ppm(const Framebuffer *fb, const char *path);

/* ── Color helpers ─────────────────────────────────────────────────────── */

/**
 * @brief Construct an opaque ARGB color from RGB components.
 * @param r Red component (0–255).
 * @param g Green component (0–255).
 * @param b Blue component (0–255).
 * @return Color value as 0xFFRRGGBB.
 */
static inline uint32_t fb_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

#endif /* RENDERING_SOFTWARE_RENDERER_H */
