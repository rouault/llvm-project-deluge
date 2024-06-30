#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdfil.h>

int main()
{
    unsigned count;
    for (count = 1000; count--;) {
        char** array = mmap(0, 16384, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANON, -1, 0);
        unsigned index;
        for (index = 16384/sizeof(char*); index--;)
            array[index] = "hello";
        munmap(array, 16384);
    }
    return 0;
}

