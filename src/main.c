#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "tdmm.h"

int main(int argc, char* argv[]){
    (void)argc;
    (void)argv;
    
    t_init(FIRST_FIT);
    
    char* string = (char*)t_malloc(20);
    strcpy(string, "Hello, World!");
    printf("%s\n", string);
    
    printf("Page size: %zu\n", page_size);
    printf("Requested size: %zu\n", requested_size);
    printf("Total size: %zu\n", total_size);
    printf("%% memory utilization: %.2f%%\n", (double)requested_size / total_size * 100);
    return 0;
}