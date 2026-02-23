#include "tdmm.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <inttypes.h>

static header* headers_start;
static header *headers_end;
static size_t requested_size;
static size_t total_size;
static alloc_strat_e strategy;
static long page_size;

static header* find_free_block(size_t size) {
    header* current = headers_start;
    header* best_fit = NULL;
    header* worst_fit = NULL;

    while(current != NULL) {
        if(current->is_free && current->size >= size) {
            if(strategy == FIRST_FIT) {
                return current;
            } else if(strategy == BEST_FIT) {
                if(best_fit == NULL || current->size < best_fit->size) {
                    best_fit = current;
                }
            } else if(strategy == WORST_FIT) {
                if(worst_fit == NULL || current->size > worst_fit->size) {
                    worst_fit = current;
                }
            }
        }
        current = current->next;
    }

    if(strategy == BEST_FIT) {
        return best_fit;
    } else if(strategy == WORST_FIT) {
        return worst_fit;
    }
    
    return NULL;
}

void t_init(alloc_strat_e strat) {
	strategy = strat;
	page_size = sysconf(_SC_PAGESIZE);
	header* initial_block = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    
    if(initial_block == NULL) {
        fprintf(stderr, "Error: failed to initialize allocator\n");
        exit(0);
    }
    
    initial_block->size = page_size - sizeof(header);
    initial_block->is_free = true;
    initial_block->is_marked = false;
    initial_block->next = NULL;
    
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
    if(block == NULL) {
        size_t size_needed = aligned_size + sizeof(header);
        size_t allocation_size = ((size_needed + page_size - 1) / page_size) * page_size;
        header* new_block = mmap(NULL, allocation_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        
        if(new_block == NULL) {
            fprintf(stderr, "Error: failed to allocate memory\n");
            exit(0);
        }
        
        new_block->size = allocation_size - sizeof(header);
        new_block->is_free = true;
        new_block->is_marked = false;
        new_block->next = NULL;
        
        headers_end->next = new_block;
        headers_end = new_block;
        block = new_block;
        
        total_size += allocation_size;
    }
    
    if (block->size >= aligned_size + sizeof(header) + 4) {
        header* new_block = (header*)((char*)block + sizeof(header) + aligned_size);
        new_block->size = block->size - aligned_size - sizeof(header);
        new_block->is_free = true;
        new_block->is_marked = false;
        new_block->next = block->next;
        
        block->size = aligned_size;
        block->next = new_block;
        
        if(headers_end == block) headers_end = new_block;
    }
    
    block->is_free = false;
    requested_size += aligned_size;
    return (char*)block + sizeof(header);
}

static void merge_blocks(header* block){
    if((char*)block->next != (char*)block + sizeof(header) + block->size) return;
    
    block->size += sizeof(header) + block->next->size;
    block->next = block->next->next;
}

static void merge_free_blocks() {
    header* current = headers_start;
    while(current && current->next) {
        if(current->is_free && current->next->is_free) merge_blocks(current);
        else current = current->next;
    }
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

static void mark(uintptr_t* ptr) {
    header* current = headers_start;
    while(current) {
        uintptr_t block_start = (uintptr_t)((char*)current + sizeof(header));
        uintptr_t block_end = block_start + current->size;
        
        if((uintptr_t)ptr >= block_start && (uintptr_t)ptr < block_end) {
            current->is_marked = true;
            break;
        }
        
        current = current->next;
    }
}

extern char **environ;
void t_gcollect(void) {
    void* stack_bottom = __builtin_frame_address(0);
    void* stack_top = environ;
    
    uintptr_t* current = (uintptr_t*)stack_bottom;
    uintptr_t* end = (uintptr_t*)stack_top;
    
    while(current < end) {
        mark((uintptr_t*) *current);
        current++;
    }
    
    header* current_block = headers_start;
    while (current_block) {
        if (!current_block->is_free) {
            if(!current_block->is_marked){
                current_block->is_free = true;
                requested_size -= current_block->size;
            }
            else current_block->is_marked = false;
        }
        current_block = current_block->next;
    }
    
    merge_free_blocks();
}

void t_display_stats() {
    printf("Page size: %zu\n", page_size);
    printf("Requested size: %zu\n", requested_size);
    printf("Total size: %zu\n", total_size);
    printf("%% memory utilization: %.2f%%\n", (double)requested_size / total_size * 100);
}
