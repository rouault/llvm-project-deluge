#include <stdio.h>

static int x = 42;

int* foo(void)
{
    printf("called foo.\n");
    return &x;
}

int bar = 666;
