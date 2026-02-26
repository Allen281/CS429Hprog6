#include <bits/time.h>
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
    FILE* malloc_graph = fopen("malloc_times.csv", "w");
    FILE* usage_graph = fopen("usage_stats.csv", "w");
    
    struct timespec start, end;
    double average_malloc_time = 0.0;
    const size_t MAX_SIZE = 1024*1024*8;
    const size_t MIN_SIZE = 1;
    const size_t INTERVAL = 1024;
    
    struct timespec total_start;
    clock_gettime(CLOCK_MONOTONIC, &total_start);
    for(size_t size = MIN_SIZE; size <= MAX_SIZE; size += INTERVAL){
        t_init(FIRST_FIT);
        clock_gettime(CLOCK_MONOTONIC, &start);
        void* ptr = t_malloc(size);
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        double malloc_time = get_elasped_time(start, end);
        average_malloc_time += malloc_time;
        fprintf(malloc_graph, "%12zu, %13.0f\n", size, malloc_time);
        
        double cur_time = get_elasped_time(total_start, end);
        double usage = t_get_usage();
        fprintf(usage_graph, "%13.0f, %.2f%%\n", cur_time, usage);
        
        t_free(ptr);
    }
    
    t_display_stats();
    size_t data_points = (MAX_SIZE-MIN_SIZE+1)/INTERVAL;
    printf("Average t_malloc time: %.0f ns\n", average_malloc_time / data_points);
    return 0;
}