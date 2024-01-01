#include "declarer.h"
#include <stdfil.h>

void foo(void)
{
    for (__SIZE_TYPE__ i = 666; i--;)
        my_global[i] = 42 * i;
}

void bar(void)
{
    for (__SIZE_TYPE__ i = 666; i--;) {
        if (my_global[i] != 42 * i)
            zerror("Bad value in my_global");
    }
}

int main(void)
{
    foo();
    bar();
    zprintf("Thingy = %d\n", my_global[100]);
    zprintf("Good\n");
    return 0;
}

