#include <stdio.h>
#include <stdfil.h>

static __attribute__((__noinline__)) void thingy(void)
{
    printf("%d\n", *(char*)zargs());
}

int main()
{
    thingy();
    return 0;
}

