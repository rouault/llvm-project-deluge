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
    char** array = mmap(0, 16384, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANON, -1, 0);
    unsigned index;
    for (index = 16384/sizeof(char*); index--;)
        array[index] = "hello";
    mprotect(array, 16384, PROT_NONE);
    zgc_request_and_wait();
    mprotect(array, 16384, PROT_READ);
    printf("%s\n", array[0]);
    return 0;
}

