#include <stdio.h>
#include <stdfil.h>

static void thingy(void)
{
    printf("%d\n", *(char*)zargs());
}

int main()
{
    thingy();
    return 0;
}

