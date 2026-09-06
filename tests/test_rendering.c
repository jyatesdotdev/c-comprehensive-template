/**
 * @file test_rendering.c
 * @brief Tests for the software framebuffer renderer.
 */
#include "check.h"
#include "rendering/software_renderer.h"

#include <stdio.h>
#include <unistd.h>

int main(void) {
    CHECK(fb_create(NULL, 8, 8) == ERR_INVALID_ARG);

    Framebuffer fb;
    CHECK(fb_create(&fb, 0, 8) == ERR_INVALID_ARG);
    CHECK(fb_create(&fb, 8, 8) == ERR_OK);
    fb_clear(&fb, 0xFF000000u);
    fb_set_pixel(&fb, 1, 1, 0x00FF00FFu);
    CHECK(fb.pixels[(size_t)1 * 8 + 1] == 0x00FF00FFu);
    fb_set_pixel(&fb, -1, 0, 0xFFFFFFFFu); /* clipped */
    fb_line(&fb, 0, 0, 7, 0, 0x0000FFFFu);
    fb_fill_rect(&fb, 2, 2, 2, 2, 0x00FF0000u);
    fb_circle(&fb, 4, 4, 2, 0x0000FF00u);

    char path[] = "test_rendering_out.ppm";
    (void)remove(path);
    CHECK(fb_write_ppm(&fb, path) == ERR_OK);
    CHECK(access(path, F_OK) == 0);
    (void)remove(path);
    fb_destroy(&fb);
    fb_destroy(&fb); /* double-destroy is safe */

    printf("All rendering tests passed.\n");
    return 0;
}
