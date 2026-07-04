# rendering — framebuffer and GPU pipelines (`src/rendering/`)

Two targets with different guarantees:

- **`rendering_sw`** (`software_renderer.c`) — always built, zero external
  dependencies, fully covered by tests/CI. A CPU framebuffer with 2D
  primitives and PPM output.
- **`rendering`** (`gl_pipeline.c`, `vk_pipeline.c`) — built only with
  `-DENABLE_RENDERING=ON`, requires OpenGL/Vulkan via Platform.cmake.
  **CI does not compile these files** — if you edit them, you must build
  locally with the option ON, and say so in your commit message.

## software_renderer invariants

- Pixels are `uint32_t` 0xAARRGGBB; the buffer is `width * height`
  computed in `size_t` (both dimensions widened *before* multiplying —
  int overflow at 46341x46341 is real).
- Every drawing function clips: `fb_set_pixel` bounds-checks and is the
  single write path — primitives (lines, circles) go through it rather
  than poking the buffer, so clipping stays centralized.
- `fb_write_ppm` checks `fwrite` and the final `fclose` (a failed close
  loses buffered pixels — ERR_IO, not shrug).

## Integration guidance

Transform math belongs in `math/` — `mat4_perspective`, `mat4_look_at`,
and friends exist precisely so pipeline code doesn't hand-roll matrices.
New rendering features should consume Mat4/Vec3/Quat rather than adding
local math.

## C lessons this module encodes

- Optional-dependency code is quarantined at the *target* level (separate
  library, CMake option), not with #ifdef soup inside shared files.
- Binary file formats (PPM) are written with explicit sizes and checked
  writes; the header is text, the payload is raw bytes — don't let
  locale-dependent formatting near either.
