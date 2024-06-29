#include <stdfil.h>
#include <stdio.h>

static void thingy(void) { }

int main()
{
    int* x = (int*)zcall(thingy, NULL, ZC_RET_BYTES);
    *x = 42;
    printf("%d\n", *x);
    return 0;
}

