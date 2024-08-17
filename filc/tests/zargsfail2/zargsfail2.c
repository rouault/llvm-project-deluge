#include <stdio.h>
#include <stdfil.h>

static __attribute__((__noinline__)) void thingy(int x)
{
    int* ptr = zargs();
    *ptr = 42;
    printf("%d\n", *ptr);
}

int main()
{
    thingy(42);
    return 0;
}

