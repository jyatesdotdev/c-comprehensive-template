/**
 * @file rendering_demo.c
 * @brief Software rendering demo — draws shapes and writes a PPM image.
 *
 * Demonstrates: framebuffer creation, Bresenham line drawing, filled
 * rectangles, midpoint circle, and PPM output. No external deps needed.
 */
#include "rendering/software_renderer.h"
#include <stdio.h>

int main(void) {
    Framebuffer fb;
    if (fb_create(&fb, 320, 240) != ERR_OK) {
        fprintf(stderr, "Failed to create framebuffer\n");
        return 1;
    }

    /* Dark background */
    fb_clear(&fb, fb_rgb(20, 20, 30));

    /* Filled rectangles */
    fb_fill_rect(&fb, 10, 10, 80, 60, fb_rgb(200, 50, 50)); /* red  */
    fb_fill_rect(&fb, 50, 40, 80, 60, fb_rgb(50, 200, 50)); /* green */
    fb_fill_rect(&fb, 90, 70, 80, 60, fb_rgb(50, 50, 200)); /* blue  */

    /* Lines forming a triangle */
    uint32_t white = fb_rgb(255, 255, 255);
    fb_line(&fb, 200, 20, 280, 180, white);
    fb_line(&fb, 280, 180, 120, 180, white);
    fb_line(&fb, 120, 180, 200, 20, white);

    /* Circle */
    fb_circle(&fb, 250, 100, 40, fb_rgb(255, 200, 0));

    /* Cross-hairs at center */
    uint32_t gray = fb_rgb(100, 100, 100);
    fb_line(&fb, 160, 0, 160, 239, gray);
    fb_line(&fb, 0, 120, 319, 120, gray);

    const char *path = "render_output.ppm";
    if (fb_write_ppm(&fb, path) == ERR_OK)
        printf("Wrote %dx%d image to %s\n", fb.width, fb.height, path);
    else fprintf(stderr, "Failed to write %s\n", path);

    fb_destroy(&fb);
    return 0;
}
