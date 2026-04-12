/**
 * @file leak_detect.c
 * @brief Allocation tracker using a singly-linked list of records.
 *
 * Must NOT include leak_detect.h with ENABLE_LEAK_DETECT active, otherwise
 * the macro replacements would recurse. We include the header after undefining.
 */
#undef ENABLE_LEAK_DETECT
#include "memory/leak_detect.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct AllocRecord {
    void              *ptr;
    size_t             size;
    const char        *file;
    int                line;
    struct AllocRecord *next;
} AllocRecord;

static AllocRecord *g_head;
static size_t       g_total_allocs;
static size_t       g_total_frees;

void leak_detect_init(void) {
    g_head = NULL;
    g_total_allocs = 0;
    g_total_frees  = 0;
}

/**
 * @brief Record a new allocation in the tracking list.
 * @param ptr   Allocated pointer.
 * @param size  Allocation size in bytes.
 * @param file  Source file of the allocation call.
 * @param line  Source line of the allocation call.
 */
static void record_add(void *ptr, size_t size, const char *file, int line) {
    AllocRecord *rec = (AllocRecord *)malloc(sizeof(AllocRecord));
    if (!rec) { fprintf(stderr, "leak_detect: out of memory for tracking\n"); return; }
    rec->ptr  = ptr;
    rec->size = size;
    rec->file = file;
    rec->line = line;
    rec->next = g_head;
    g_head = rec;
    g_total_allocs++;
}

/**
 * @brief Remove an allocation record by pointer, logging if untracked.
 * @param ptr  Pointer to remove from the tracking list.
 */
static void record_remove(void *ptr) {
    AllocRecord **cur = &g_head;
    while (*cur) {
        if ((*cur)->ptr == ptr) {
            AllocRecord *tmp = *cur;
            *cur = tmp->next;
            free(tmp);
            g_total_frees++;
            return;
        }
        cur = &(*cur)->next;
    }
    fprintf(stderr, "leak_detect: free of untracked pointer %p\n", ptr);
}

void *leak_detect_malloc(size_t size, const char *file, int line) {
    void *ptr = malloc(size);
    if (ptr) record_add(ptr, size, file, line);
    return ptr;
}

void *leak_detect_calloc(size_t count, size_t size, const char *file, int line) {
    void *ptr = calloc(count, size);
    if (ptr) record_add(ptr, count * size, file, line);
    return ptr;
}

void *leak_detect_realloc(void *ptr, size_t size, const char *file, int line) {
    if (ptr) record_remove(ptr);
    void *newptr = realloc(ptr, size);
    if (newptr) record_add(newptr, size, file, line);
    return newptr;
}

void leak_detect_free(void *ptr, const char *file, int line) {
    (void)file; (void)line;
    if (!ptr) return;
    record_remove(ptr);
    free(ptr);
}

size_t leak_detect_report(void) {
    size_t leaks = 0;
    size_t leaked_bytes = 0;
    for (AllocRecord *r = g_head; r; r = r->next) {
        fprintf(stderr, "  LEAK: %zu bytes at %p allocated at %s:%d\n",
                r->size, r->ptr, r->file, r->line);
        leaks++;
        leaked_bytes += r->size;
    }
    if (leaks) {
        fprintf(stderr, "leak_detect: %zu leak(s), %zu bytes total\n", leaks, leaked_bytes);
    }
    fprintf(stderr, "leak_detect: %zu allocs, %zu frees\n", g_total_allocs, g_total_frees);
    return leaks;
}

void leak_detect_shutdown(void) {
    AllocRecord *r = g_head;
    while (r) {
        AllocRecord *next = r->next;
        free(r);
        r = next;
    }
    g_head = NULL;
}
