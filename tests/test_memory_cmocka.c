/**
 * @file test_memory_cmocka.c
 * @brief Arena and pool tests using the cmocka framework.
 *
 * Demonstrates cmocka integration: group fixtures, assert macros,
 * and cmocka_run_group_tests runner.
 *
 * Build with: -DUSE_CMOCKA=ON (requires cmocka installed on system)
 */
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "memory/arena.h"
#include "memory/pool.h"

/* ── Fixtures ─────────────────────────────────────────────────────────────── */

static int setup_arena(void **state) {
    Arena *a = test_malloc(sizeof(Arena));
    assert_int_equal(arena_init(a, 4096), 0);
    *state = a;
    return 0;
}

static int teardown_arena(void **state) {
    arena_destroy((Arena *)*state);
    test_free(*state);
    return 0;
}

/* ── Arena tests ──────────────────────────────────────────────────────────── */

static void test_arena_basic_alloc(void **state) {
    Arena *a = (Arena *)*state;
    void  *p = arena_alloc(a, 64, 8);
    assert_non_null(p);
    assert_int_equal((uintptr_t)p % 8, 0);
}

static void test_arena_reset(void **state) {
    Arena *a = (Arena *)*state;
    void  *first = arena_alloc(a, 128, 8);
    arena_reset(a);
    void *second = arena_alloc(a, 128, 8);
    assert_ptr_equal(first, second);
}

/* ── Pool tests (no fixture, inline setup) ────────────────────────────────── */

static void test_pool_reuse(void **state) {
    (void)state;
    Pool p;
    assert_int_equal(pool_init(&p, 32, 4), 0);
    void *a = pool_alloc(&p);
    pool_free(&p, a);
    void *b = pool_alloc(&p);
    assert_ptr_equal(a, b);
    pool_destroy(&p);
}

/* ── Runner ───────────────────────────────────────────────────────────────── */

int main(void) {
    const struct CMUnitTest arena_tests[] = {
        cmocka_unit_test_setup_teardown(test_arena_basic_alloc, setup_arena, teardown_arena),
        cmocka_unit_test_setup_teardown(test_arena_reset, setup_arena, teardown_arena),
    };
    const struct CMUnitTest pool_tests[] = {
        cmocka_unit_test(test_pool_reuse),
    };

    int fail = 0;
    fail |= cmocka_run_group_tests(arena_tests, NULL, NULL);
    fail |= cmocka_run_group_tests(pool_tests, NULL, NULL);
    return fail;
}
