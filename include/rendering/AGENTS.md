# rendering public API (`include/rendering/`)

Software framebuffer (always available) and GL/Vulkan pipelines (optional).
General header rules: `include/AGENTS.md`; target split and invariants:
`src/rendering/AGENTS.md`.

## Consumer contract

- `software_renderer.h` — link `rendering_sw` (always built). Pixels are
  `uint32_t` 0xAARRGGBB; coordinates are ints with (0,0) top-left. All
  drawing clips to the framebuffer — out-of-bounds draws are safe no-ops,
  not errors. `fb_create`/`fb_destroy` pair; `fb_write_ppm` writes a
  binary P6 file (viewable almost anywhere) and reports write failures.
- `gl_pipeline.h` / `vk_pipeline.h` — only meaningful with
  `-DENABLE_RENDERING=ON` (links the `rendering` target); **CI never
  compiles these**, so treat them as reference material and build locally
  before relying on changes to them.
- Transform math comes from `math/` (`mat4_perspective`, `mat4_look_at`,
  `quat_to_mat4`) — don't hand-roll matrices in rendering code.
