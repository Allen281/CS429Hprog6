#include <bits/types/clockid_t.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "tdmm.h"

// Helper method to get the elapsed time in nanoseconds between two timespecs
double get_elasped_time(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec)*1e9 + (end.tv_nsec - start.tv_nsec);
}

int main(){
    t_init(FIRST_FIT);
    
    struct timespec start, end;
    
    printf("Size (Bytes) | t_malloc (ns) | t_free (ns)\n");
    printf("-------------------------------------------\n");
    
    double average_malloc_time = 0.0;
    double average_free_time = 0.0;
    for(size_t size = 1; size <= 1024 * 8; size++){
        clock_gettime(CLOCK_MONOTONIC, &start);
        void* ptr = t_malloc(size);
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        double malloc_time = get_elasped_time(start, end);
        average_malloc_time += malloc_time;
        
        clock_gettime(CLOCK_MONOTONIC, &start);
        t_free(ptr);
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        double free_time = get_elasped_time(start, end);
        average_free_time += free_time;
        
        printf("%12zu | %13.0f | %11.0f\n", size, malloc_time, free_time);
    }
    printf("-------------------------------------------\n");
    
    t_display_stats();
    printf("Average t_malloc time: %.0f ns\n", average_malloc_time / 8192);
    printf("Average t_free time: %.0f ns\n", average_free_time / 8192);
    return 0;
}