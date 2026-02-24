#include "tdmm.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

static header* headers_start;
static header *headers_end;
static size_t requested_size;
static size_t total_size;
static alloc_strat_e strategy;
static long page_size;

static void set_block_state(header* block, bool is_free, bool is_marked, size_t size, header* next) {
    block->is_free = is_free;
    block->is_marked = is_marked;
    block->size = size;
    block->next = next;    
}

static header* find_free_block(size_t size) {
    header* current = headers_start;
    header* rslt = NULL;

    while(current != NULL) {
        if(current->is_free && current->size >= size) {
            if(strategy == FIRST_FIT) {
                return current;
            } else if(strategy == BEST_FIT) {
                if(rslt == NULL || current->size < rslt->size) rslt = current;
            } else if(strategy == WORST_FIT) {
                if(rslt == NULL || current->size > rslt->size) rslt = current;
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
    
    set_block_state(initial_block, true, false, page_size - sizeof(header), NULL);
    
    headers_start = initial_block;
    headers_end = initial_block;
    requested_size = 0;
    total_size = page_size;
}

void *t_malloc(size_t size) {
    if(size == 0) return NULL;
    
    size_t aligned_size = size;
    if(aligned_size%4 != 0) aligned_size += 4 - (aligned_size % 4);
    
    header* block = find_free_block(aligned_size);
    if(block == MAP_FAILED) {
        size_t size_needed = aligned_size + sizeof(header);
        size_t allocation_size = ((size_needed + page_size - 1) / page_size) * page_size;
        void* endptr = (char*)headers_end + sizeof(header) + headers_end->size;
        header* new_block = mmap(endptr, allocation_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        
        if(new_block == NULL) {
            fprintf(stderr, "Error: failed to allocate memory\n");
            exit(1);
        }
        
        set_block_state(new_block, true, false, allocation_size - sizeof(header), NULL);
        
        headers_end->next = new_block;
        headers_end = new_block;
        block = new_block;
        
        total_size += allocation_size;
    }
    
    if (block->size >= aligned_size + sizeof(header) + 4) {
        header* new_block = (header*)((char*)block + sizeof(header) + aligned_size);
        set_block_state(new_block, true, false, block->size - aligned_size - sizeof(header), block->next);
        
        block->size = aligned_size;
        block->next = new_block;
        
        if(headers_end == block) headers_end = new_block;
    }
    
    block->is_free = false;
    requested_size += aligned_size;
    return (char*)block + sizeof(header);
}

static void merge_blocks(header* block){
    if(!block || !block->next) return;
    if((char*)block->next != (char*)block + sizeof(header) + block->size) return;
    if(!block->is_free || !block->next->is_free) return;
    
    block->size += sizeof(header) + block->next->size;
    block->next = block->next->next;
}

void t_free(void *ptr) {
   	if(ptr == NULL) return;
    
    header* current = headers_start;
    bool found = false;
    header* prev = NULL;
    while(current) {
        void* current_block = (char*)current + sizeof(header);
        
        if(current_block == ptr) {
            if(current->is_free) {
                fprintf(stderr, "Error: attempted to free an already freed block\n");
                return;
            }
            found = true;
            break;
        }
        
        prev = current;
        current = current->next;
    }
    
    if(!found) {
        fprintf(stderr, "Error: attempted to free a pointer that was not allocated\n");
        return;
    }
    
    header* block = (header*)((char*)ptr - sizeof(header));
    block->is_free = true;
    requested_size -= block->size;
    
    merge_blocks(block);
    merge_blocks(prev);
}

void t_display_stats() {
    printf("Page size: %zu\n", page_size);
    printf("Requested size: %zu\n", requested_size);
    printf("Total size: %zu\n", total_size);
    printf("%% memory utilization: %.2f%%\n", (double)requested_size / total_size * 100);
}
