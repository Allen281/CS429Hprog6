#ifndef TDMM_H
#define TDMM_H

#include <stddef.h>
#include <stdbool.h>

typedef struct header header;
typedef struct allocator allocator;

struct header {
    size_t size;
    bool is_free;
    header* next;
};

typedef enum {
  FIRST_FIT,
  BEST_FIT,
  WORST_FIT,
} alloc_strat_e;

static header* headers_start;
static header *headers_end;
static size_t requested_size;
static size_t total_size;
static alloc_strat_e strategy;
static long page_size;

/**
 * Initializes the memory allocator with the given strategy.
 *
 * @param strat The strategy to use for memory allocation.
 */
void t_init(alloc_strat_e strat);

/**
 * Allocates a block of memory of the given size.
 *
 * @param size The size of the memory block to allocate.
 * @return A pointer to the allocated memory block fails.
 */
void *t_malloc(size_t size);

/**
 * Frees the given memory block.
 *
 * @param ptr The pointer to the memory block to free. This must be a pointer returned by t_malloc.
 */
void t_free(void *ptr);

/**
 * Performs basic garbage collection by scanning the stack and heap managed by t_malloc and t_free.
 */
void t_gcollect(void);

#endif // TDMM_H
