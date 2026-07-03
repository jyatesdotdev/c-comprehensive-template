/** Arena allocator demo */
#include "memory/arena.h"
#include <stdio.h>

int main(void) {
    Arena a;
    if (arena_init(&a, 4096) != ERR_OK) {
        fprintf(stderr, "init failed\n");
        return 1;
    }

    int *nums = arena_alloc(&a, 10 * sizeof(int), _Alignof(int));
    for (int i = 0; i < 10; i++) nums[i] = i * i;
    for (int i = 0; i < 10; i++) printf("nums[%d] = %d\n", i, nums[i]);

    arena_destroy(&a);
    return 0;
}
