#include <stdio.h>

#include "binary_heap.h"
#include "logger.h"

int main(void) {
    logger_initialize_stream(stdout);
    puts("main.c is configured and running.");
    logger_close();
    return 0;
}
