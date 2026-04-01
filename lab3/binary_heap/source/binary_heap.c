#include <stdio.h>

#include "asserts.h"
#include "logger.h"

typedef struct {
    int key;
} bin_heap_node;

typedef struct {
    bin_heap_node** node_arr;
    
} bin_heap;