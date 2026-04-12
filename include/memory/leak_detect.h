/**
 * @file leak_detect.h
 * @brief Debug allocator that tracks allocations and reports leaks.
 *
 * When ENABLE_LEAK_DETECT is defined, malloc/free/calloc/realloc are replaced
 * with tracking wrappers that record file and line of each allocation.
 * Call leak_detect_report() at program exit to print unfreed allocations.
 */
#ifndef MEMORY_LEAK_DETECT_H
#define MEMORY_LEAK_DETECT_H

#include <stddef.h>

/**
 * @brief Initialize the leak detector. Call once at program start.
 */
void leak_detect_init(void);

/**
 * @brief Tracked malloc — records source location.
 * @param size  Number of bytes to allocate.
 * @param file  Source file of the allocation (typically __FILE__).
 * @param line  Source line of the allocation (typically __LINE__).
 * @return Pointer to allocated memory, or NULL on failure.
 */
void *leak_detect_malloc(size_t size, const char *file, int line);

/**
 * @brief Tracked calloc — records source location.
 * @param count Number of elements.
 * @param size  Size of each element in bytes.
 * @param file  Source file of the allocation.
 * @param line  Source line of the allocation.
 * @return Pointer to zero-initialized memory, or NULL on failure.
 */
void *leak_detect_calloc(size_t count, size_t size, const char *file, int line);

/**
 * @brief Tracked realloc — records source location.
 * @param ptr   Pointer to previously allocated memory, or NULL.
 * @param size  New size in bytes.
 * @param file  Source file of the allocation.
 * @param line  Source line of the allocation.
 * @return Pointer to reallocated memory, or NULL on failure.
 */
void *leak_detect_realloc(void *ptr, size_t size, const char *file, int line);

/**
 * @brief Tracked free — removes allocation from tracking.
 * @param ptr   Pointer to free.
 * @param file  Source file of the free call.
 * @param line  Source line of the free call.
 */
void  leak_detect_free(void *ptr, const char *file, int line);

/**
 * @brief Print all unfreed allocations to stderr.
 * @return Number of leaked allocations (0 = clean).
 */
size_t leak_detect_report(void);

/**
 * @brief Release all internal tracking state.
 */
void leak_detect_shutdown(void);

#ifdef ENABLE_LEAK_DETECT
#define malloc(sz)        leak_detect_malloc((sz), __FILE__, __LINE__)
#define calloc(n, sz)     leak_detect_calloc((n), (sz), __FILE__, __LINE__)
#define realloc(p, sz)    leak_detect_realloc((p), (sz), __FILE__, __LINE__)
#define free(p)           leak_detect_free((p), __FILE__, __LINE__)
#endif

#endif /* MEMORY_LEAK_DETECT_H */
