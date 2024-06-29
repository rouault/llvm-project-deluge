#include <stdfil.h>
#include <stdio.h>

static int thingy(void) { return 666; }

int main()
{
    int* x = (int*)zcall(thingy, NULL, ZC_RET_BYTES);
    *x = 42;
    printf("%d\n", *x);
    return 0;
}

