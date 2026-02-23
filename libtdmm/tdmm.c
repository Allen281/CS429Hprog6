#include "tdmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

static allocator* alloc;

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
	// TODO: Implement this
	strategy = strat;
	header* initial_block = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    
    if(initial_block == NULL) {
        fprintf(stderr, "Error: failed to initialize allocator\n");
        exit(0);
    }
    
    initial_block->size = 1024 - sizeof(header);
    initial_block->is_free = true;
    initial_block->next = NULL;
    headers_start = initial_block;
    headers_end = initial_block;
    requested_size = 0;
    total_size = 1024;
}

void *t_malloc(size_t size) {
    if(size == 0) return NULL;
    
    size_t aligned_size = size;
    if(aligned_size%4 != 0) aligned_size += 4 - (aligned_size % 4);
    
    header* block = find_free_block(aligned_size);
    if(block == NULL) { 
        header* new_block = mmap(NULL, aligned_size + sizeof(header), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if(new_block == NULL) {
            fprintf(stderr, "Error: failed to allocate memory\n");
            exit(0);
        }
        new_block->size = aligned_size;
        new_block->is_free = false;
        new_block->next = NULL;
        
        header* end = headers_end;
        end->next = new_block;
        headers_end = new_block;
        total_size += aligned_size + sizeof(header);
        requested_size += size;
        return (char*)new_block + sizeof(header);
    }
    
    if (block->size >= aligned_size + sizeof(header) + 4) {
        header* new_block = (header*)((char*)block + sizeof(header) + aligned_size);
        new_block->size = block->size - aligned_size - sizeof(header);
        new_block->is_free = true;
        new_block->next = block->next;
        
        block->size = aligned_size;
        block->next = new_block;
    }
    
    block->is_free = false;
    total_size += aligned_size + sizeof(header);
    requested_size += aligned_size;
    return (char*)block + sizeof(header);
}

void t_free(void *ptr) {
   	if(ptr == NULL) {
        return;
    }
    
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
    total_size -= block->size + sizeof(header);
    
    if(block->next != NULL && block->next->is_free && (char*)block + sizeof(header) + block->size == (char*)block->next) {
        block->size += sizeof(header) + block->next->size;
        block->next = block->next->next;
    }
    
    if(prev != NULL && prev->is_free && (char*)prev + sizeof(header) + prev->size == (char*)block) {
        prev->size += sizeof(header) + block->size;
        prev->next = block->next;
    } 
}

void t_gcollect(void) {
  	// TODO: Implement this
}
