#include <stdfil.h>
#include "utils.h"

int main(void)
{
    int x;

    float (*func)(char x) = (float(*)(char))opaque(&x);

    zprint("spoko");
    float y = func(42);
    zprint("kurcze");
    return 0;
}

