# Public Headers (`include/`)

Everything under `include/` is public API. If a declaration isn't meant for
consumers, it belongs in the module's `src/` directory (see
`src/networking/socket_internal.h` for the internal-header pattern).

## Rules for every header

1. **Include guard matches the path**: `include/math/vec.h` →
   `MATH_VEC_H`, closed with `#endif /* MATH_VEC_H */`.
2. **Doxygen on every public declaration.** `@file` + `@brief` at the top;
   `@brief`, `@param`, `@return` on functions. Document the *contract*, not
   the implementation: valid ranges, who owns/frees returned memory, what
   each ErrorCode means for this call, thread-safety expectations.
3. **Headers are self-contained.** Include exactly what your declarations
   need — no more. A consumer must be able to include any single header
   first and compile.
4. **Fallible functions return `ErrorCode`** (`core/error.h`); results go
   out through pointer parameters named `out` / `out_*`. Infallible small
   types (Vec3, Quat) pass and return by value instead.
5. **Transparent structs by default** — callers can stack-allocate and the
   template favors teachability. Use an opaque type (`typedef struct X X;`
   + `x_create`/`x_destroy`) only when invariants must be unbreakable from
   outside (see `hpc/thread_pool.h`, `networking/event_loop.h`). Say which
   fields are internal with comments if the struct is transparent.
6. **`static inline` only for tiny, hot, obviously-correct functions**
   (see `math/scalar.h`, the `matx_get`/`matx_set` accessors). Anything
   with a failure path or a loop belongs in a `.c` file.
7. **Lifecycle naming**: `x_init`/`x_destroy` for caller-allocated structs,
   `x_create`/`x_destroy` for heap-allocated opaque types. Destroy functions
   are safe on NULL and safe to call twice.

## Naming

- Types `PascalCase`; functions `<prefix>_<action>` where the prefix is the
  file or type name (`arena_init`, `sv_trim`, `bq_push`); macros and enum
  constants `UPPER_SNAKE`.
- Enums that are bitmasks document it and use powers of two (`EV_READ`).

## When you change a header

Update the Doxygen comment in the same edit as the signature — a stale
contract is worse than none. Then check `docs/ARCHITECTURE.md` tables and
the module's example, which exist to show the API as it really is.
