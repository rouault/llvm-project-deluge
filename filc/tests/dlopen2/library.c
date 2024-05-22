#include <stdio.h>
#include <stdfil.h>

static int x = 42;

int* foo(void)
{
    printf("called foo.\n");
    return &x;
}

int bar = 666;

void thingy(void) __attribute__((constructor));
void thingy(void)
{
    zprintf("thingy called.");
    bar = 1410;
}
