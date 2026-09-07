/**
 * @file test_rendering.c
 * @brief Tests for the software framebuffer renderer.
 */
#include "check.h"
#include "rendering/software_renderer.h"

#include <stdio.h>
#include <unistd.h>

int main(void) {
    CHECK(framebuffer_create(NULL, 8, 8) == ERR_INVALID_ARG);

    Framebuffer fb;
    CHECK(framebuffer_create(&fb, 0, 8) == ERR_INVALID_ARG);
    CHECK(framebuffer_create(&fb, 8, 8) == ERR_OK);
    framebuffer_clear(&fb, 0xFF000000u);
    framebuffer_set_pixel(&fb, 1, 1, 0x00FF00FFu);
    CHECK(fb.pixels[(size_t)1 * 8 + 1] == 0x00FF00FFu);
    framebuffer_set_pixel(&fb, -1, 0, 0xFFFFFFFFu); /* clipped */
    framebuffer_line(&fb, 0, 0, 7, 0, 0x0000FFFFu);
    framebuffer_fill_rect(&fb, 2, 2, 2, 2, 0x00FF0000u);
    framebuffer_circle(&fb, 4, 4, 2, 0x0000FF00u);

    char path[] = "test_rendering_out.ppm";
    (void)remove(path);
    CHECK(framebuffer_write_ppm(&fb, path) == ERR_OK);
    CHECK(access(path, F_OK) == 0);
    (void)remove(path);
    framebuffer_destroy(&fb);
    framebuffer_destroy(&fb); /* double-destroy is safe */

    printf("All rendering tests passed.\n");
    return 0;
}
