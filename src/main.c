#include <stdio.h>
#include <stdbool.h>
#include <sys/mman.h>

typedef struct header header;
typedef struct allocator allocator;

typedef enum{
    FIRST_FIT,
    BEST_FIT,
    WORST_FIT
}policy;

struct header{
    size_t size;
    bool is_free;
    header* next;
};

struct allocator{
    header* headers_start;
    header* headers_end;
    size_t requested_size;
    size_t total_size;
};

void* tmalloc(size_t size, policy p);
void tfree(void* pointer);

static allocator* alloc;

static void init_alloc(){
    alloc = mmap(NULL, sizeof(allocator), PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);   
}

int main(int argc, char* argv[]){
    argc = argc;
    argv = argv;
    init_alloc();
    
    return 0;
}

static header* find_free_block(size_t size, policy p) {
    header* current = alloc->headers_start;
    header* best_fit = NULL;
    header* worst_fit = NULL;

    while(current != NULL) {
        if(current->is_free && current->size >= size) {
            if(p == FIRST_FIT) {
                return current;
            } else if(p == BEST_FIT) {
                if(best_fit == NULL || current->size < best_fit->size) {
                    best_fit = current;
                }
            } else if(p == WORST_FIT) {
                if(worst_fit == NULL || current->size > worst_fit->size) {
                    worst_fit = current;
                }
            }
        }
        current = current->next;
    }

    if(p == BEST_FIT) {
        return best_fit;
    } else if(p == WORST_FIT) {
        return worst_fit;
    }
    
    return NULL;
}

void* tmalloc(size_t size, policy p) {
    if(alloc->headers_start == NULL) {
        size_t total_size = size + sizeof(header);
        if(total_size%4 != 0) total_size += 4 - (total_size % 4);
        
        header* initial_block = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
        initial_block->size = size;
        initial_block->is_free = false;
        initial_block->next = NULL;
        
        alloc->headers_start = initial_block;
        alloc->headers_end = initial_block;
        alloc->total_size = total_size;
        alloc->requested_size = size;
        
        return (char*)initial_block + sizeof(header);
    }
    
    header* block = find_free_block(size, p);
    if(block == NULL) {
        size_t total_size = size + sizeof(header);
        if(total_size%4 != 0) total_size += 4 - (total_size % 4);
        
        header* new_block = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
        new_block->size = size;
        new_block->is_free = false;
        new_block->next = NULL;
        
        header* end = alloc->headers_end;
        end->next = new_block;
        alloc->headers_end = new_block;
        alloc->total_size += total_size;
        alloc->requested_size += size;
        return (char*)new_block + sizeof(header);
    }
    
    header* new_block = (header*)((char*)block + sizeof(header) + size);
    new_block->size = block->size - size - sizeof(header);
    new_block->is_free = true;
    new_block->next = block->next;
    
    block->size = size;
    block->is_free = false;
    block->next = new_block;
    
    alloc->total_size += size + sizeof(header);
    alloc->requested_size += size;
    return (char*)block + sizeof(header);
}

void tfree(void* pointer) {
    if(pointer == NULL) {
        return;
    }
    
    header* current = alloc->headers_start;
    bool found = false;
    header* prev = NULL;
    while(current) {
        void* current_block = (char*)current + sizeof(header);
        
        if(current_block == pointer) {
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
    
    header* block = (header*)((char*)pointer - sizeof(header));
    block->is_free = true;
    
    alloc->requested_size -= block->size;
    alloc->total_size -= block->size + sizeof(header);
    
    if(block->next != NULL && block->next->is_free) {
        block->size += sizeof(header) + block->next->size;
        block->next = block->next->next;
    }
    
    if(prev != NULL && prev->is_free) {
        prev->size += sizeof(header) + block->size;
        prev->next = block->next;
    } 
}