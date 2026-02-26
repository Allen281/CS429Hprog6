#include "tdmm.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define IS_FREE(x) ((x)->size & 1)
#define SET_FREE(x, y) ((x)->size = ((x)->size & ~1ULL) | (y))

#define GET_SIZE(x) ((x)->size & ~1ULL)
#define SET_SIZE(x, y) ((x)->size = (y) | IS_FREE(x))

static header* headers_start;
static header *headers_end;
static size_t requested_size;
static size_t total_size;
static alloc_strat_e strategy;
static long page_size;
static size_t data_structure_overhead;

static void set_block_state(header* block, int is_free, size_t size, header* next, header* prev) {
    block->size = size | is_free;
    block->next = next;
    block->prev = prev;
}

static header* find_free_block(size_t size) {
    header* current = headers_start;
    header* rslt = NULL;

    while(current != NULL) {
        size_t s = GET_SIZE(current);
        if(IS_FREE(current) && s >= size) {
            if(strategy == FIRST_FIT) {
                return current;
            } else if(strategy == BEST_FIT) {
                if(rslt == NULL || s < GET_SIZE(rslt)) rslt = current;
            } else if(strategy == WORST_FIT) {
                if(rslt == NULL || s > GET_SIZE(rslt)) rslt = current;
            }
        }
        current = current->next;
    }

    return rslt;
}

void t_init(alloc_strat_e strat) {
	strategy = strat;
	page_size = sysconf(_SC_PAGESIZE);
	header* initial_block = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    
    if(initial_block == MAP_FAILED) {
        fprintf(stderr, "Error: failed to initialize allocator\n");
        exit(0);
    }
    
    set_block_state(initial_block, true, page_size - sizeof(header), NULL, NULL);
    
    headers_start = initial_block;
    headers_end = initial_block;
    requested_size = 0;
    total_size = page_size;
    data_structure_overhead = sizeof(header);
}

void *t_malloc(size_t size) {
    if(size == 0) return NULL;
    
    size_t aligned_size = (size + 3) & ~3;
    
    header* block = find_free_block(aligned_size);
    if(block == NULL) {
        size_t size_needed = aligned_size + sizeof(header);
        size_t allocation_size = (size_needed + page_size - 1) & ~(page_size - 1);
        void* endptr = (char*)headers_end + sizeof(header) + GET_SIZE(headers_end);
        header* new_block = mmap(endptr, allocation_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        
        if(new_block == MAP_FAILED) {
            fprintf(stderr, "Error: failed to allocate memory\n");
            exit(1);
        }
        
        set_block_state(new_block, true, allocation_size - sizeof(header), NULL, headers_end);
        
        headers_end->next = new_block;
        headers_end = new_block;
        block = new_block;
        
        total_size += allocation_size;
        data_structure_overhead += sizeof(header);
    }
    
    size_t block_size = GET_SIZE(block);
    if (block_size >= aligned_size + sizeof(header) + 4) {
        header* new_block = (header*)((char*)block + sizeof(header) + aligned_size);
        set_block_state(new_block, true, block_size - aligned_size - sizeof(header), block->next, block);
        
        block_size = aligned_size;
        block->next = new_block;
        
        if(headers_end == block) headers_end = new_block;
        data_structure_overhead += sizeof(header);
    }
    
    block->size = block_size;
    requested_size += block_size;
    return (char*)block + sizeof(header);
}

static void merge_blocks(header* block){
    if(!block || !block->next) return;
    if((char*)block->next != (char*)block + sizeof(header) + GET_SIZE(block)) return;
    if(!IS_FREE(block) || !IS_FREE(block->next)) return;
    
    if(block->next == headers_end) headers_end = block;
    
    block->size += sizeof(header) + GET_SIZE(block->next);
    block->next = block->next->next;
    data_structure_overhead -= sizeof(header);
}

void t_free(void *ptr) {
   	if(ptr == NULL) return;
    
    header* block = (header*)((char*)ptr - sizeof(header));
    SET_FREE(block, 1);
    requested_size -= GET_SIZE(block);
    
    merge_blocks(block);
    merge_blocks(block->prev);
}

void t_display_stats() {
    printf("Page size: %zu bytes\n", page_size);
    printf("Total bytes requested from sys: %zu bytes\n", total_size);
    printf("Data structure overhead: %zu bytes (%.5f%%)\n", data_structure_overhead, (double)data_structure_overhead / total_size * 100);
}

double t_get_usage(){ return (double)requested_size / total_size * 100;}