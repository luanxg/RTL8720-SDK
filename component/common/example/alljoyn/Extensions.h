#ifndef EXTENSIONS_H
#define EXTENSIONS_H

#include <stdio.h>
#include <stdlib.h>

//log function
#define PRINT_LOG       1
#define log(fmat,...) \
    do { \
        if(PRINT_LOG){ \
        printf(fmat "\n", ##__VA_ARGS__); \
        } \
    } while (0)

//count of array elements
#define count(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
        
#endif //EXTENSIONS_H