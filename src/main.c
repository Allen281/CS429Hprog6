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
    //t_free(string);
    
    t_display_stats();
    return 0;
}