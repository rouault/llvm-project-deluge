#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

int main()
{
    int* p = (int*)malloc(100);
    p[4] = 42;
    uintptr_t ip = (uintptr_t)p;
    ip += 16;
    int* p2 = (int*)ip;
    printf("value = %d\n", *p2);
    return 0;
}

