#include <stdio.h>
#include <stdlib.h>

__attribute__((__noinline__)) static void foo(void)
{
}

__attribute__((__noinline__)) static void bar(void)
{
    exit(0);
}

int main()
{
    int* p = malloc(42);
    p[0] = 666;
    p[1] = 1410;
    foo();
    printf("values: %d, %d\n", p[0], p[1]);
    bar();
    printf("Should not get here (1)\n");
    p[666] = 42;
    printf("Should not get here (2)\n");
    return 0;
}

