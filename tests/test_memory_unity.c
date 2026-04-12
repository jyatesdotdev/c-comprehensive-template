/**
 * @file test_memory_unity.c
 * @brief Arena and pool tests using the Unity test framework.
 *
 * Demonstrates Unity integration: setUp/tearDown, TEST_ASSERT macros,
 * and the UNITY_BEGIN/UNITY_END runner pattern.
 */
#include "unity.h"
#include "memory/arena.h"
#include "memory/pool.h"

static Arena arena;
static Pool pool;

void setUp(void) {
    /* Fresh allocators for each test */
    arena_init(&arena, 4096);
    pool_init(&pool, 64, 8);
}

void tearDown(void) {
    arena_destroy(&arena);
    pool_destroy(&pool);
}

/* ── Arena tests ──────────────────────────────────────────────────────────── */

void test_arena_alloc_returns_aligned_pointer(void) {
    void *p = arena_alloc(&arena, 32, 16);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_UINT(0, (uintptr_t)p % 16);
}

void test_arena_reset_allows_reuse(void) {
    void *first = arena_alloc(&arena, 128, 8);
    arena_reset(&arena);
    void *second = arena_alloc(&arena, 128, 8);
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_EQUAL_PTR(first, second);
}

void test_arena_returns_null_when_full(void) {
    /* 4096-byte arena, allocate until exhausted */
    void *p;
    int count = 0;
    while ((p = arena_alloc(&arena, 512, 8)) != NULL) count++;
    TEST_ASSERT_GREATER_THAN(0, count);
    TEST_ASSERT_NULL(p);
}

/* ── Pool tests ───────────────────────────────────────────────────────────── */

void test_pool_alloc_and_free(void) {
    void *a = pool_alloc(&pool);
    void *b = pool_alloc(&pool);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_EQUAL(a, b);
    pool_free(&pool, a);
    void *c = pool_alloc(&pool);
    TEST_ASSERT_EQUAL_PTR(a, c); /* reuses freed slot */
}

void test_pool_exhaustion(void) {
    /* 8 blocks of 64 bytes */
    for (int i = 0; i < 8; i++)
        TEST_ASSERT_NOT_NULL(pool_alloc(&pool));
    TEST_ASSERT_NULL(pool_alloc(&pool));
}

/* ── Runner ───────────────────────────────────────────────────────────────── */

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_arena_alloc_returns_aligned_pointer);
    RUN_TEST(test_arena_reset_allows_reuse);
    RUN_TEST(test_arena_returns_null_when_full);
    RUN_TEST(test_pool_alloc_and_free);
    RUN_TEST(test_pool_exhaustion);
    return UNITY_END();
}
