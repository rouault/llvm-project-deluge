#include <stdfil.h>
#include <stdio.h>

static int thingy(void) { return 666; }

int main()
{
    int args;
    int x = zcall_value(thingy, &args + 10, int);
    printf("%d\n", x);
    return 0;
}

